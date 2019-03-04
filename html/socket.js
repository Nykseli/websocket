

window.onload = function() {

    let sock = new WebSocket("ws://localhost:8888");

    sock.onmessage = function(e) {
        console.log("On message");
        console.log(e);
    }

    sock.onopen = function(e) {
        console.log("on open");
        console.log(e);
    }

    sock.onclose = function(e) {
        console.log("on close");
        console.log(e);
    }
}
