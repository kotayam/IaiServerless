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

func invokeHandler(w http.ResponseWriter, r *http.Request) {
	// extract function name
	funcName := strings.TrimPrefix(r.URL.Path, "/")
	if funcName == "" {
		http.Error(w, "IaiServerless Gateway is active. Request a function like /hello", http.StatusBadRequest)
		return
	}

	// create path to binary
	safeName := filepath.Base(funcName)
	binPath := fmt.Sprintf("../samples/%s.bin", safeName)

	// check if binary exists
	if _, err := os.Stat(binPath); os.IsNotExist(err) {
		http.Error(w, fmt.Sprintf("Function binary not found: %s", binPath), http.StatusNotFound)
		return
	}

	// spawn host loader process with binary
	cmd := exec.Command("../host/host_loader", binPath)

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
	portFlag := flag.String("port", "8080", "The port for the gateway to listen on")
	flag.Parse()
	port := fmt.Sprintf(":%s", *portFlag)

	http.HandleFunc("/", invokeHandler)
	fmt.Printf("🚀 IaiServerless Gateway listening on http://localhost%s...\n", port)
	log.Fatal(http.ListenAndServe(port, nil))
}
