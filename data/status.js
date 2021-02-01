// -*- tcl -*-

function connect(name, msgfunc) {
    var wsurl = "ws" + document.URL.match("s?://[^?#]+/") + name
    if ("WebSocket" in window) {
	ws = new WebSocket(wsurl)
    } else if ("MozWebSocket" in window) {
	ws = new MozWebSocket(wsurl)
    }
    if (ws) {
	ws.onmessage = msgfunc
	ws.onclose = teardown
    }
}

function wsdata(evt) {
    var message = JSON.parse(evt.data)
    for (var type in message) {
	var w = document.getElementById(type)
	if (w) {
	    if (w.tagName == "INPUT" && w.type == "checkbox") {
		w.indeterminate = false
		w.checked = (message[type] == 1)
	    } else {
		w.innerHTML = message[type]
	    }
	}
    }
}

function teardown(evt) {
    if (evt.code != 1001) {
	var w = document.getElementById("socklost")
	if (w) {
	    w.style.display = "block"
	}
    }
}

function neutralall() {
    var list = document.getElementsByTagName("input");
    for (var w of list) {
	if (w.type == "checkbox") w.indeterminate = true
    }
}
