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
		binPath := fmt.Sprintf("../samples/%s.bin", safeName)
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
	var stdoutBuf bytes.Buffer
	cmd.Stdout = &stdoutBuf
	cmd.Stderr = os.Stderr
	err := cmd.Run()

	if err != nil {
		// if KVM crashes, return a 500 error with the host loader's output
		http.Error(w, fmt.Sprintf("VM execution failed: %v", err), http.StatusInternalServerError)
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
