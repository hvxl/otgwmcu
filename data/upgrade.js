// -*- tcl -*-

let busy = false

let result = [
    "Success",
    "Not enough memory",
    "Failed to open the firmware file",
    "Invalid firmware file format",
    "Wrong data size in hex file",
    "Bad checksum in hex file",
    "A previous firmware upgrade is running",
    "Hex file contains unexpected data",
    "Failed to reset the PIC",
    "Too many retries",
    "Too many mismatches",
    "The selected firmware is for a different PIC"
]

function versionsort(list) {
    return list.sort(function(a, b) {
	return b.version.localeCompare(a.version, undefined, {numeric: true})
    })
}

function buildtables(name, data) {
    let w = document.getElementById(name)
    if (w) {
	let parent = w.parentNode
	let tables = [w]
	let count = Object.keys(data).length
	for (let i = tables.length; i < count; i++) {
	    let clone = w.cloneNode(true)
	    // Avoid duplicate id's
	    clone.removeAttribute("id")
	    tables.push(clone)
	}
	let i = 0
	for (let name in data) {
	    let w = tables[i++]
	    buildtable(w, name, versionsort(data[name]))
	    parent.appendChild(w)
	}
    }
}

function buildtable(table, name, data) {
    let title = table.getElementsByTagName("span")[0]
    if (title) title.innerText = name
    let add = table.querySelector("[name=pic]")
    if (add) add.value = name
    let w = table.getElementsByTagName("tbody")[0]
    let orig = w.getElementsByTagName("tr")[0]
    let tbody = document.createElement("tbody")
    for (let i = 0; i < data.length; i++) {
	let row = orig.cloneNode(true)
	let info = data[i]
	for (let item in info) {
	    let list = row.querySelectorAll('[name="' + item +'"], [data-name="' + item + '"]')
	    let value = info[item]
	    if (item == "name") value = name + "/" + value
	    for (let j = 0; j < list.length; j++) {
		let e = list[j]
		switch (e.tagName) {
		  case "INPUT":
		    e.value = value
		  default:
		    e.innerHTML = info[item]
		    break
		}
	    }
	}
	tbody.appendChild(row)
    }
    w.parentNode.replaceChild(tbody, w)
}

function showconfig() {
    w = document.getElementById('processor')
    if (w) {
	if (processor != "unknown") {
	    w.innerText = processor.toUpperCase()
	} else {
	    w.innerText = processor
	}
    }
    w = document.getElementById('firmware')
    if (w) {
	let [fw, version] = firmware
	if (version != "") {
	    w.innerText = fw + " " + version
	} else {
	    w.innerText = "unknown"
	}
    }
}

window.addEventListener("load", () => {
    let wsurl = "ws" + document.URL.match("s?://[^?#]+/") + "download.ws"
    if ("WebSocket" in window) {
        ws = new WebSocket(wsurl)
    } else if ("MozWebSocket" in window) {
        ws = new MozWebSocket(wsurl)
    }
    if (ws) {
        ws.onmessage = wsdata
        // ws.onclose = teardown
    }
    buildtables('filelist', ls)
    showconfig()
    // Intercept all form submisions
    for (let form of document.getElementsByTagName("form")) {
	form.addEventListener("submit", handleform)
    }
})

function handleform(e) {
    if (e.submitter.value == "download") {
	download(e.target, e.submitter)
	if (e.preventDefault) e.preventDefault()
	return false
    } else {
	return true
    }
}

function download(form, button) {
    let w, data = new FormData(form)
    data.set(button.name, button.value)
    fetch(form.action, {method: form.method, body: data})
    w = document.getElementById("percent")
    if (w) w.style.width = "0%"
    w = document.getElementById("status")
    if (w) w.innerText = "Please wait ..."
    overlay(true)
}

function overlay(show = false) {
    if (show != busy) {
	let w = document.getElementById("overlay")
	if (w) w.style.display = show ? "" : "none"
	busy = show
    }
}

function wsdata(evt) {
    let message = JSON.parse(evt.data)
    let status = []
    let cfgupdate = false
    for (let type in message) {
	let w, value = message[type]
	switch (type) {
	  case "result":
	    setTimeout(overlay, 5000)
	    status.push(result[value])
	    break
	  case "errors":
	  case "retries":
	    status.push(type + ": " + value)
	    break
	  case "processor":
	  case "firmware":
	    // Set the global variable
	    window[type] = value
	    cfgupdate = true
	    break;
	  default:
	    w = document.getElementById(type)
	    if (w) {
		if (w.tagName == "DIV") {
		    overlay(true)
		    w.style.width = value + "%"
		} else {
		    w.innerHTML = value
		}
	    }
	}
    }
    if (status.length > 0) {
	let w = document.getElementById("status")
	if (w) w.innerText = status.join(", ")
    }
    if (cfgupdate) showconfig()
}
