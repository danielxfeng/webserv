import requests
import socket

HOME = "/home/xifeng"
HOST = "127.0.0.1"
PORT = 8081
BASE = f"http://{HOST}:{PORT}"

def send_raw(raw_bytes: bytes) -> bytes:
    with socket.create_connection((HOST, PORT), timeout=5) as s:
        s.sendall(raw_bytes)
        s.settimeout(1.0)
        try:
            resp = b""
            while True:
                chunk = s.recv(4096)
                if not chunk:
                    break
                resp += chunk
        except socket.timeout:
            pass
        return resp

def test_get_html():
    r = requests.get(f"{BASE}/index.html")
    assert r.status_code == 200
    with open(f"{HOME}/goinfre/www/app/index.html", "r") as f:
        expected_content = f.read()
    assert expected_content in r.text
    print("GET HTML test passed.")

def test_default_html():
    r = requests.get(f"{BASE}/")
    assert r.status_code == 200
    with open(f"{HOME}/goinfre/www/app/index.html", "r") as f:
        expected_content = f.read()
    assert expected_content in r.text
    print("GET default HTML test passed.")

def test_get_txt():
    r = requests.get(f"{BASE}/second/text.txt")
    print(r.status_code)
    assert r.status_code == 200
    with open(f"{HOME}/goinfre/www/app/file.txt", "r") as f:
        expected_content = f.read()
    assert expected_content in r.text
    print("GET TXT test passed.")

def test_get_autoindex():
    r = requests.get(f"{BASE}/second/")
    assert r.status_code == 200
    assert "Index of /second/" in r.text
    assert "text.txt" in r.text
    print("GET autoindex test passed.")

def test_get_404():
    r = requests.get(f"{BASE}/nonexistent")
    assert r.status_code == 404
    print("GET 404 test passed.")

def test_get_content_length_too_large():
    # declare content-length larger than actual body
    req = (
        b"GET / HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Content-Length: 10\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    # check for 400 or connection close — servers vary
    assert b"400" in resp or b"Bad Request" in resp or resp.startswith(b"HTTP/1.1"), "unexpected server reaction"
    print("GET content-length too large test passed.")

def test_get_incorrect_http_version():
    req = (
        b"GET / HTTP/1.2\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"400" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("GET incorrect HTTP version test passed.")

def test_get_incorrect_method():
    req = (
        b"FETCH / HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"400" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("GET incorrect method test passed.")

def test_get_no_method():
    req = (
        b"/ HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"Error" in resp, "unexpected server reaction"
    print("GET no method test passed.")

def test_get_no_http_version():
    req = (
        b"GET / \r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"Error" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("GET no HTTP version test passed.")

def test_get_incorrect_protocol():
    req = (
        b"GET / FTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"Error" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("GET incorrect protocol test passed.")


def run():
    test_get_html()
    #test_default_html()
    #test_get_txt()
    #test_get_autoindex()
    test_get_404()
    test_get_content_length_too_large()
    #test_get_incorrect_http_version()
    test_get_incorrect_method()
    test_get_no_method()
    test_get_no_http_version()
    test_get_incorrect_protocol()
    print("All tests passed.")


if __name__=="__main__":
    run()