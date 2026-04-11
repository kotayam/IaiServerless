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
	"strings"
)

var runtimeMode string

func invokeHandler(w http.ResponseWriter, r *http.Request) {
	// extract function name
	funcName := strings.TrimPrefix(r.URL.Path, "/")
	if funcName == "" {
		http.Error(w, "IaiServerless Gateway is active. Request a function like /hello", http.StatusBadRequest)
		return
	}

	// create path to binary
	safeName := filepath.Base(funcName)

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
	case "process":
		binPath := fmt.Sprintf("../samples/%s_proc", safeName)
		if _, err := os.Stat(binPath); os.IsNotExist(err) {
			http.Error(w, fmt.Sprintf("Function binary not found: %s", binPath), http.StatusNotFound)
			return
		}
		cmd = exec.Command(binPath)
	case "docker":
		containerName := fmt.Sprintf("iai_%s", safeName)
		cmd = exec.Command("docker", "run", "--rm", containerName)
	default:
		http.Error(w, "Invalid runtime mode configured", http.StatusInternalServerError)
		return
	}

	// capture stdout and stderr from loader
	var stdoutBuf, stderrBuf bytes.Buffer
	cmd.Stdout = &stdoutBuf
	cmd.Stderr = &stderrBuf
	err := cmd.Run()

	if err != nil {
		// if host loader returns non-zero, it means the guest function failed
		w.WriteHeader(http.StatusInternalServerError)
		// return both the output and the error message to the client
		w.Write([]byte(fmt.Sprintf("MicroVM Execution Failed:\n%s\n%s", stdoutBuf.String(), stderrBuf.String())))
		return
	}

	// send binary output directly to the client
	w.WriteHeader(http.StatusOK)
	w.Write(stdoutBuf.Bytes())
}

func main() {
	flag.StringVar(&runtimeMode, "runtime", "kvm", "Execution runtime: kvm, process, or docker")
	portFlag := flag.String("port", "8080", "The port for the gateway to listen on")
	flag.Parse()
	port := fmt.Sprintf(":%s", *portFlag)

	http.HandleFunc("/", invokeHandler)
	fmt.Printf("IaiServerless Gateway listening on http://localhost%s...\n", port)
	fmt.Printf("Active Runtime: %s\n", runtimeMode)
	log.Fatal(http.ListenAndServe(port, nil))
}
