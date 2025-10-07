import os
import sys

"""
A very simple Python CGI application that handles GET and POST requests.
"""

MAX_CONTENT_LENGTH = 1000 * 1024 * 1024

BASE_DIR = "/home/xifeng/www/cgi/python"

class CGIError(Exception):
    error_map = {
        400: "Bad Request",
        404: "Not Found",
        405: "Method Not Allowed",
        411: "Length Required",
        413: "Payload Too Large",
        415: "Unsupported Media Type",
        500: "Internal Server Error",
        505: "HTTP Version Not Supported"
    }

    def __init__(self, code, message):
        super().__init__(message)
        self.message = message
        self.code = code
        self.description = self.error_map.get(code, "Unknown Error")

class Storage:
    def __init__(self):
        os.makedirs(BASE_DIR, exist_ok=True)

    def _safe_path(self, name: str) -> str:
        p = os.path.realpath(os.path.join(BASE_DIR, name))
        base_real = os.path.realpath(BASE_DIR)
        if not p.startswith(base_real + os.sep):
            raise CGIError(400, "Bad Request: Invalid path")
        return p

    def get_value(self, key):
        file_path = self._safe_path(key)
        if not os.path.isfile(file_path):
            raise CGIError(404, f"Not Found: Key '{key}' does not exist")
        try:
            with open(file_path, 'r') as f:
                return f.read().strip()
        except Exception as e:
            raise CGIError(500, f"Internal Server Error: {str(e)}")

    def set_value(self, key, value):
        file_path = self._safe_path(key)
        if not key or not value:
            raise CGIError(400, "Bad Request: Key and Value cannot be empty")
        if os.path.isfile(file_path) and os.path.exists(file_path):
            raise CGIError(400, f"Bad Request: Key '{key}' already exists")
        try:
            with open(file_path, 'w') as f:
                f.write(value)
        except Exception as e:
            raise CGIError(500, f"Internal Server Error: {str(e)}")

class Response:
    def __init__(self, message, status=200, status_message="OK", error=None):
        self.status = error.code if error else status
        self.status_message = error.description if error else status_message
        self.message = message if error is None else error.message

    def write(self):
        body = self.message.encode("utf-8")
        headers = (
            f"Status: {self.status} {self.status_message}\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            f"Content-Length: {len(body)}\r\n"
            "\r\n"
        )
        with open("python", "w") as err:
            err.write(f"Response headers: {headers}\n")
            err.write(f"Response Body: {body}\n")
        sys.stdout.write(headers + self.message)
        sys.stdout.buffer.flush()

class Header:
    def __init__(self):
        self.method = os.environ.get("REQUEST_METHOD", "")
        self.chunked = os.environ.get("HTTP_TRANSFER_ENCODING", "") == "chunked"
        if (self.method == "GET" or self.method == "DELETE"):
            self.content_length = 0
        elif (self.chunked):
            self.content_length = MAX_CONTENT_LENGTH + 1
        else:
            try:
                self.content_length = int(os.environ.get("CONTENT_LENGTH") or 0)
            except ValueError:
                raise CGIError(400, "Bad Request: Invalid Content Length")
        self.content_type = os.environ.get("CONTENT_TYPE", "")
        self.path_info = os.environ.get("PATH_INFO", "")

        if self.method not in ["GET", "POST"]:
            raise CGIError(405, "Method Not Allowed")
        if self.content_length < 0:
            raise CGIError(400, "Bad Request: Invalid Content Length")
        if self.content_length > MAX_CONTENT_LENGTH:
            raise CGIError(413, "Payload Too Large")
        if self.content_type and not self.content_type.startswith("text/plain"):
            raise CGIError(415, "Unsupported Media Type")
        if self.path_info and not self.path_info.startswith("/"):
            raise CGIError(400, "Bad Request: Invalid Path Info")

class Request:
    def __init__(self):
        self.storage = Storage()
        self.headers = Header()
        self.body = self._get_body()

    def _get_body(self):
        if self.headers.method == "POST" and self.headers.content_length > 0:
            received = 0
            last_chunk = b""
            try:
                while received < self.headers.content_length:
                    chunk = sys.stdin.buffer.read(
                        min(4096, self.headers.content_length - received)
                    )
                    if not chunk:
                        break
                    last_chunk = chunk  # overwrite → keep only tail
                    received += len(chunk)

                if not self.chunked and received != self.headers.content_length:
                    raise CGIError(400, "Bad Request: Incomplete request body")

                return last_chunk.decode("utf-8", errors="replace") # keep only tail for disk space efficiency
            except Exception as e:
                raise CGIError(400, f"Bad Request: {str(e)}")
        return ""

    def _handle_get(self):
        if (self.headers.path_info == "/"):
            return Response("Welcome to the Python CGI application!")

        key = self.headers.path_info.lstrip('/')
        value = self.storage.get_value(key)
        return Response(value)

    def _handle_post(self):
        if self.headers.content_length == 0:
            raise CGIError(411, "Length Required")

        if not self.headers.path_info:
            raise CGIError(400, "Bad Request: No Path Info for POST")

        key = self.headers.path_info.lstrip('/')
        if not key:
            raise CGIError(400, "Bad Request: Key cannot be empty")

        if not key.isalnum():
            raise CGIError(400, "Bad Request: Key must be alphanumeric")

        try:
            value = self.body.strip()
            if not value:
                raise CGIError(400, "Bad Request: Value cannot be empty")
            if not value.isprintable():
                raise CGIError(400, "Bad Request: Value must be printable")

            value += "\n This is a sample cgi application, so the data is chunked."
            self.storage.set_value(key, value)
            return Response(f"Stored value for '{key}': {value}", status=201)

        except Exception as e:
            raise CGIError(500, f"Internal Server Error: {str(e)}")

    def dispatch(self):
        if self.headers.method == "GET":
            return self._handle_get()
        elif self.headers.method == "POST":
            return self._handle_post()
        else:
            raise CGIError(405, "Method Not Allowed")


def main():
    try:
        request = Request()
        response = request.dispatch()
        response.write()
    except CGIError as e:
        response = Response(f"Error: {e.message}", error=e)
        response.write()
        return
    except Exception as e:
        response = Response(f"Error: {str(e)}", error=CGIError(500, "Internal Server Error"))
        response.write()
        return

if __name__ == "__main__":
    main()
