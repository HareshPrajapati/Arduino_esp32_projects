

function sortTable(l){
var e=document.getElementById("theader"),n=e.cells[l].dataset.order||"1",s=0-(n=parseInt(n,10));
e.cells[l].dataset.order=s;var t,a=document.getElementById("tbody"),r=a.rows,d=[];
for(t=0;t<r.length;t++)d.push(r[t]);
for(d.sort(function(e,t){
    var a=e.cells[l].dataset.value,r=t.cells[l].dataset.value;
    return l?(a=parseInt(a,10),(r=parseInt(r,10))<a?s:a<r?n:0):r<a?s:a<r?n:0
}),t=0;t<d.length;t++)a.appendChild(d[t])
}


function sendData(led) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
    document.getElementById("LEDState").innerHTML = this.responseText;
    }
    };
    xhttp.open("GET", "setLED?LEDstate="+led, true);
    xhttp.send();
    }

function _(el) {
    return document.getElementById(el);
    }


function progress() {
    var x = document.getElementById("progressBar");
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
    x.value = parseInt(this.responseText);
    console.log("progress" + x.value);
    }
    }
    xhttp.open("GET", "showPer", true);
    xhttp.send();
    }


setInterval(progress, 100);


