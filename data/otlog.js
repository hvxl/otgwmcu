// -*- tcl -*-
// Limit scrollback to 64 Mb
const maxkb = 65536

function connect(name, msgfunc) {
    var wsurl = "ws" + document.URL.match("s?://[^?#]+/") + name;
    if ("WebSocket" in window) {
	ws = new WebSocket(wsurl);
    } else if ("MozWebSocket" in window) {
	ws = new MozWebSocket(wsurl);
    }
    if (ws) {
        ws.onmessage = msgfunc;
        ws.onclose = teardown;
    }
}

function wsdata(evt) {
    var str = evt.data;
    var w = document.getElementById("log")
    if (w) {
	var p = w.parentElement
	var bottom = p.scrollHeight - p.scrollTop < p.clientHeight + 5
	w.innerHTML += str + "\n"
	var len = w.innerHTML.length
	if (len > maxkb * 1024) {
	    x = len - maxkb * 1000
	    w.innerHTML = w.innerHTML.substring(w.innerHTML.indexOf("\n", x) + 1)
	}
	if (bottom) w.scrollIntoView(false)
    }
}

function teardown(evt) {
    if (evt.code != 1001) {
        var w = document.getElementById("socklost");
        if (w) {
            w.style.display = "block";
        }
    }
}
