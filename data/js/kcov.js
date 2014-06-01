window.onload = function () {
	Tempo.prepare('source-code').render(data);

	elem = document.getElementById('header-percent-covered')

	elem.className = toCoverPercentString(header.covered, header.instrumented);
	elem.innerHTML = ((header.covered / header.instrumented) * 100).toFixed(1) + "%";	    

	document.getElementById('header-command').innerHTML = header.command;
	document.getElementById('window-title').innerHTML = "Coverage report - " + header.command;
	document.getElementById('header-date').innerHTML = header.date;
	document.getElementById('header-covered').innerHTML = header.covered
	document.getElementById('header-instrumented').innerHTML = header.instrumented
}

function toCoverPercentString (covered, instrumented) {
	perc = (covered / instrumented) * 100;

	if (perc <= percent_low)
		return "coverPerLeftLo";
	else if (perc >= percent_high)
		return "coverPerLeftHi";

	return "coverPerLeftMed";
}
