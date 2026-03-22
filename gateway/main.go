package main

import (
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
	output, err := cmd.CombinedOutput()
	if err != nil {
		// if KVM crashes, return a 500 error with the host loader's output
		http.Error(w, fmt.Sprintf("VM execution failed: %v\nOutput:\n%s", err, string(output)), http.StatusInternalServerError)
		return
	}

	// send binary output directly to the client
	w.WriteHeader(http.StatusOK)
	w.Write(output)
}

func main() {
	http.HandleFunc("/", invokeHandler)

	port := ":8080"
	fmt.Printf("🚀 IaiServerless Gateway listening on http://localhost%s...\n", port)
	log.Fatal(http.ListenAndServe(port, nil))
}
