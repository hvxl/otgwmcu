// -*- tcl -*-

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
	    w.innerText = fw + " version " + version
	} else {
	    w.innerText = "unknown"
	}
    }
}

function buildtable(table, name, data) {
    let title = table.getElementsByTagName("caption")[0]
    title.innerText = name
    let w = table.getElementsByTagName("tbody")[0]
    let orig = w.getElementsByTagName("tr")[0]
    let tbody = document.createElement("tbody")
    for (let i = 0; i < data.length; i++) {
	let row = orig.cloneNode(true)
	let info = data[i]
	for (let item in info) {
	    let list = row.querySelectorAll('[name="' + item +'"]')
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
