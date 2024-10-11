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
	ws.onopen = online
	ws.onclose = teardown
    }
}

function wsdata(evt) {
    var message = JSON.parse(evt.data)
    for (var type in message) {
	var w = document.getElementById(type)
	if (w) {
	    if (w.tagName == "INPUT") {
	       if (w.type == "checkbox") {
		   w.indeterminate = false
		   w.checked = (message[type] == 1)
	       } else {
		   w.value = message[type]
		   if (w.onchange) w.onchange()
	       }
	    } else {
		w.innerHTML = message[type]
	    }
	}
    }
}

function online(evt) {
    w = document.getElementById("websock")
    if (w) {
	w.checked = 1
    }
}

function teardown(evt) {
    if (evt.code != 1001) {
	var w = document.getElementById("socklost")
	if (w) {
	    w.style.display = "block"
	}
	w = document.getElementById("websock")
	if (w) {
	    w.checked = 0
	}
    }
}

function neutralall() {
    var list = document.getElementsByTagName("input")
    for (var w of list) {
	if (w.type == "checkbox") w.indeterminate = true
    }
}
