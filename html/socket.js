

let sock;
window.onload = function() {

    sock = new WebSocket("ws://localhost:8888");
    sock.onmessage = function(e) {
        console.log("On message");
        console.log(e);
    }

    sock.onopen = function(e) {
        console.log("on open");
        // Send the message to server right after the connection is opened
        sock.send("Socket echo");
        console.log(e);
    }

    sock.onclose = function(e) {
        console.log("on close");
        console.log(e);
    }

    sock.onerror = function(e) {
        console.log("on error");
        console.log(e);
    }
}
