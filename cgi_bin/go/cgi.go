package main

//
// A very simple Python CGI application that handles GET and POST requests.
//

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

const (
	MaxContentLength = 1000 * 1024 * 1024
	BaseDir          = "~/go_user_files"
)

// CGIError represents an HTTP error
type CGIError struct {
	Code        int
	Message     string
	Description string
}

func (e *CGIError) Error() string {
	return e.Message
}

var errorMap = map[int]string{
	400: "Bad Request",
	404: "Not Found",
	405: "Method Not Allowed",
	411: "Length Required",
	413: "Payload Too Large",
	415: "Unsupported Media Type",
	500: "Internal Server Error",
	505: "HTTP Version Not Supported",
}

// writeResponse outputs CGI headers + body
func writeResponse(status int, message string) {
	desc := errorMap[status]
	if desc == "" {
		desc = "Unknown"
	}
	body := []byte(message)
	fmt.Fprintf(os.Stdout, "Status: %d %s\r\n", status, desc)
	fmt.Fprintf(os.Stdout, "Content-Type: text/plain; charset=utf-8\r\n")
	fmt.Fprintf(os.Stdout, "Content-Length: %d\r\n", len(body))
	fmt.Fprintf(os.Stdout, "\r\n")
	os.Stdout.Write(body)
}

// safePath ensures no path traversal
func safePath(name string) (string, error) {
	base, _ := filepath.Abs(BaseDir)
	full, _ := filepath.Abs(filepath.Join(BaseDir, name))
	if !strings.HasPrefix(full, base+string(os.PathSeparator)) {
		return "", &CGIError{400, "Invalid path", errorMap[400]}
	}
	return full, nil
}

// GET handler
func handleGet(path string) {
	if path == "/" || path == "" {
		writeResponse(200, "Welcome to the Go CGI application!")
		return
	}
	key := strings.TrimPrefix(path, "/")
	filePath, err := safePath(key)
	if err != nil {
		writeResponse(err.(*CGIError).Code, err.Error())
		return
	}
	data, err := os.ReadFile(filePath)
	if err != nil {
		writeResponse(404, "Not Found: Key does not exist")
		return
	}
	writeResponse(200, string(data))
}

// POST handler (reads whole body, keeps tail only)
func handlePost(path string, contentLength int) {
	if path == "" || path == "/" {
		writeResponse(400, "Bad Request: No Path Info for POST")
		return
	}
	key := strings.TrimPrefix(path, "/")
	if key == "" {
		writeResponse(400, "Bad Request: Key cannot be empty")
		return
	}

	// Prepare storage dir
	os.MkdirAll(BaseDir, 0755)
	filePath, err := safePath(key)
	if err != nil {
		writeResponse(err.(*CGIError).Code, err.Error())
		return
	}
	if _, err := os.Stat(filePath); err == nil {
		writeResponse(400, "Bad Request: Key already exists")
		return
	}

	// Read stdin in chunks, keep last
	reader := bufio.NewReader(os.Stdin)
	var lastChunk []byte
	received := 0
	chunkSize := 4096
	for received < contentLength {
		toRead := chunkSize
		if contentLength-received < chunkSize {
			toRead = contentLength - received
		}
		buf := make([]byte, toRead)
		n, err := io.ReadFull(reader, buf)
		if err != nil && err != io.ErrUnexpectedEOF {
			writeResponse(400, "Bad Request: Failed reading body")
			return
		}
		if n == 0 {
			break
		}
		lastChunk = buf[:n] // overwrite → keep only tail
		received += n
	}

	if received != contentLength {
		writeResponse(400, "Bad Request: Incomplete request body")
		return
	}

	value := strings.TrimSpace(string(lastChunk))
	if value == "" {
		writeResponse(400, "Bad Request: Value cannot be empty")
		return
	}

	// Store last chunk
	err = os.WriteFile(filePath, []byte(value), 0644)
	if err != nil {
		writeResponse(500, "Internal Server Error: Failed to write file")
		return
	}

	msg := fmt.Sprintf("Stored value for '%s': %s", key, value)
	writeResponse(201, msg)
}

func main() {
	method := os.Getenv("REQUEST_METHOD")
	pathInfo := os.Getenv("PATH_INFO")
	contentLengthStr := os.Getenv("CONTENT_LENGTH")

	var contentLength int
	if contentLengthStr != "" {
		n, err := strconv.Atoi(contentLengthStr)
		if err != nil {
			writeResponse(400, "Bad Request: Invalid Content Length")
			return
		}
		contentLength = n
	}

	switch method {
	case "GET":
		handleGet(pathInfo)
	case "POST":
		if contentLength <= 0 {
			writeResponse(411, "Length Required")
			return
		}
		if contentLength > MaxContentLength {
			writeResponse(413, "Payload Too Large")
			return
		}
		handlePost(pathInfo, contentLength)
	default:
		writeResponse(405, "Method Not Allowed")
	}
}
