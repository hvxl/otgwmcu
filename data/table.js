// -*- tcl -*-

function versionsort(list) {
    return list.sort(function(a, b) {
	return b.version.localeCompare(a.version, undefined, {numeric: true})
    })
}

function buildtable(name, data) {
    var w = document.getElementById(name)
    if (w) {
	var orig = w.getElementsByTagName("tr")[0]
	var tbody = document.createElement("tbody")
	for (var i = 0; i < data.length; i++) {
	    var row = orig.cloneNode(true)
	    var info = data[i]
	    for (var item in info) {
		var list = row.querySelectorAll('[name="' + item +'"]')
		for (var j = 0; j < list.length; j++) {
		    var e = list[j]
		    switch (e.tagName) {
		      case "INPUT":
			e.value = info[item]
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
}
