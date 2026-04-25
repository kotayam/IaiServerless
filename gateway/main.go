package main

import (
	"bytes"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"time"
)

var runtimeMode string
var junctionRunPath string

func staticHandler(w http.ResponseWriter, r *http.Request) {
	http.ServeFile(w, r, "static/index.html")
}

func sourceHandler(w http.ResponseWriter, r *http.Request) {
	fn := filepath.Clean(strings.TrimPrefix(r.URL.Path, "/source/"))
	var path string
	if strings.HasPrefix(fn, "python/") {
		path = fmt.Sprintf("../samples/%s.py", fn)
	} else {
		path = fmt.Sprintf("../samples/%s.c", fn)
	}
	data, err := os.ReadFile(path)
	if err != nil {
		http.NotFound(w, r)
		return
	}
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Write(data)
}

func invokeHandler(w http.ResponseWriter, r *http.Request) {
	t0 := time.Now()

	// extract function name
	funcName := strings.TrimPrefix(r.URL.Path, "/")
	if funcName == "" {
		http.Error(w, "IaiServerless Gateway is active. Request a function like c/hello", http.StatusBadRequest)
		return
	}

	// create path to binary
	safeName := filepath.Clean(funcName)

	// run binary
	var cmd *exec.Cmd
	switch runtimeMode {
	case "kvm":
		binPath := fmt.Sprintf("../samples/%s.elf", safeName)
		if _, err := os.Stat(binPath); os.IsNotExist(err) {
			http.Error(w, fmt.Sprintf("Function binary not found: %s", binPath), http.StatusNotFound)
			return
		}
		cmd = exec.Command("../host/host_loader", binPath)
	case "native":
		if strings.HasPrefix(safeName, "python/") {
			scriptPath := fmt.Sprintf("../samples/%s.py", safeName)
			if _, err := os.Stat(scriptPath); os.IsNotExist(err) {
				http.Error(w, fmt.Sprintf("Function script not found: %s", scriptPath), http.StatusNotFound)
				return
			}
			cmd = exec.Command("python3", "../samples/python/runner.py", scriptPath)
		} else {
			binPath := fmt.Sprintf("../samples/%s_proc", safeName)
			if _, err := os.Stat(binPath); os.IsNotExist(err) {
				http.Error(w, fmt.Sprintf("Function binary not found: %s", binPath), http.StatusNotFound)
				return
			}
			cmd = exec.Command(binPath)
		}
	case "process":
		binPath := fmt.Sprintf("../samples/%s_proc", safeName)
		if _, err := os.Stat(binPath); os.IsNotExist(err) {
			http.Error(w, fmt.Sprintf("Function binary not found: %s", binPath), http.StatusNotFound)
			return
		}
		cmd = exec.Command(binPath)
	case "python":
		scriptPath := fmt.Sprintf("../samples/%s.py", safeName)
		if _, err := os.Stat(scriptPath); os.IsNotExist(err) {
			http.Error(w, fmt.Sprintf("Function script not found: %s", scriptPath), http.StatusNotFound)
			return
		}
		cmd = exec.Command("python3", "../samples/python/runner.py", scriptPath)
	case "docker":
		// Include language in container name to avoid collisions (e.g., iai_c_hello, iai_cpp_test)
		containerName := fmt.Sprintf("iai_%s", strings.ReplaceAll(safeName, "/", "_"))
		cmd = exec.Command("docker", "run", "--rm", "--network=host", "-i", containerName)
	case "junction":
		binPath := fmt.Sprintf("../samples/%s.elf", safeName)
		if _, err := os.Stat(binPath); os.IsNotExist(err) {
			http.Error(w, fmt.Sprintf("Function binary not found: %s", binPath), http.StatusNotFound)
			return
		}
		cmd = exec.Command(junctionRunPath, binPath)
	default:
		http.Error(w, "Invalid runtime mode configured", http.StatusInternalServerError)
		return
	}

	// capture stdout and stderr from loader
	var stdoutBuf, stderrBuf bytes.Buffer
	cmd.Stdout = &stdoutBuf
	cmd.Stderr = &stderrBuf
	cmd.Stdin = r.Body

	t1 := time.Now()
	err := cmd.Run()
	t2 := time.Now()
	e2e := float64(time.Since(t0).Nanoseconds()) / 1e6

	coldStart := float64(t1.Sub(t0).Nanoseconds()) / 1e6
	execTime := float64(t2.Sub(t1).Nanoseconds()) / 1e6

	// For KVM, parse precise timings emitted by host_loader on stderr.
	// For process/docker, timing.c emits X-Exec-Time (CLOCK_MONOTONIC delta)
	// and cold start is derived as: cold_start = e2e - exec_time.
	switch runtimeMode {
	case "kvm":
		for _, line := range strings.Split(stderrBuf.String(), "\n") {
			if val, ok := strings.CutPrefix(line, "X-Exec-Time: "); ok {
				execTime, _ = strconv.ParseFloat(val, 64)
				coldStart = e2e - execTime
			}
		}
	case "process", "docker", "python", "junction":
		for _, line := range strings.Split(stderrBuf.String(), "\n") {
			if val, ok := strings.CutPrefix(line, "X-Exec-Time: "); ok {
				execTime, _ = strconv.ParseFloat(val, 64)
				coldStart = e2e - execTime
			}
		}
	}

	// Set timing headers
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Access-Control-Expose-Headers", "X-Cold-Start, X-Exec-Time, X-E2E-Latency")
	w.Header().Set("X-Cold-Start", fmt.Sprintf("%.4f", coldStart))
	w.Header().Set("X-Exec-Time", fmt.Sprintf("%.4f", execTime))
	w.Header().Set("X-E2E-Latency", fmt.Sprintf("%.4f", e2e))

	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte(fmt.Sprintf("MicroVM Execution Failed:\n%s\n%s", stdoutBuf.String(), stderrBuf.String())))
		return
	}

	// Send successful response
	w.WriteHeader(http.StatusOK)
	w.Write(stdoutBuf.Bytes())
}

func main() {
	flag.StringVar(&runtimeMode, "runtime", "kvm", "Execution runtime: kvm, process, docker, or junction")
	flag.StringVar(&junctionRunPath, "junction-run", "", "Path to junction_run binary (required when runtime=junction)")
	portFlag := flag.String("port", "8080", "The port for the gateway to listen on")
	flag.Parse()

	if runtimeMode == "junction" && junctionRunPath == "" {
		log.Fatal("-junction-run flag is required when runtime=junction")
	}
	port := fmt.Sprintf(":%s", *portFlag)

	http.HandleFunc("/source/", sourceHandler)
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/" {
			staticHandler(w, r)
		} else {
			invokeHandler(w, r)
		}
	})
	fmt.Printf("IaiServerless Gateway listening on http://localhost%s...\n", port)
	fmt.Printf("Active Runtime: %s\n", runtimeMode)
	log.Fatal(http.ListenAndServe(port, nil))
}
