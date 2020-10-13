#include <Arduino.h>

const char header[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<html lang="en">
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta charset="UTF-8">
<title id="title">Web FileBrowser</title>
<style>
    h1 {
        font-family: Arial, Helvetica, sans-serif;
        border-bottom: 1px solid #c0c0c0;
        margin-bottom: 10px;
        padding-bottom: 10px;
        white-space: nowrap;
        }
    body {
        background-color: #ccffff;
        }
    table{
        font-family: Arial, Helvetica, sans-serif;
        border-collapse: collapse;
        }
    th  {
        font-family: Arial, Helvetica, sans-serif;
        cursor: pointer;
        }
    td.detailsColumn {
        font-family: Arial, Helvetica, sans-serif;
        -webkit-padding-start: 2em;
        text-align: end;
        white-space: nowrap;
        }
    a.icon {
        -webkit-padding-start: 1.5em;
        text-decoration: none;
        }
    a.icon:hover {
        text-decoration: underline;
        }
    a.file {
        background : url("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABnRSTlMAAAAAAABupgeRAAABHUlEQVR42o2RMW7DIBiF3498iHRJD5JKHurL+CRVBp+i2T16tTynF2gO0KSb5ZrBBl4HHDBuK/WXACH4eO9/CAAAbdvijzLGNE1TVZXfZuHg6XCAQESAZXbOKaXO57eiKG6ft9PrKQIkCQqFoIiQFBGlFIB5nvM8t9aOX2Nd18oDzjnPgCDpn/BH4zh2XZdlWVmWiUK4IgCBoFMUz9eP6zRN75cLgEQhcmTQIbl72O0f9865qLAAsURAAgKBJKEtgLXWvyjLuFsThCSstb8rBCaAQhDYWgIZ7myM+TUBjDHrHlZcbMYYk34cN0YSLcgS+wL0fe9TXDMbY33fR2AYBvyQ8L0Gk8MwREBrTfKe4TpTzwhArXWi8HI84h/1DfwI5mhxJamFAAAAAElFTkSuQmCC ") left top no-repeat;
        }
    a.dir {
        background : url("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAd5JREFUeNqMU79rFUEQ/vbuodFEEkzAImBpkUabFP4ldpaJhZXYm/RiZWsv/hkWFglBUyTIgyAIIfgIRjHv3r39MePM7N3LcbxAFvZ2b2bn22/mm3XMjF+HL3YW7q28YSIw8mBKoBihhhgCsoORot9d3/ywg3YowMXwNde/PzGnk2vn6PitrT+/PGeNaecg4+qNY3D43vy16A5wDDd4Aqg/ngmrjl/GoN0U5V1QquHQG3q+TPDVhVwyBffcmQGJmSVfyZk7R3SngI4JKfwDJ2+05zIg8gbiereTZRHhJ5KCMOwDFLjhoBTn2g0ghagfKeIYJDPFyibJVBtTREwq60SpYvh5++PpwatHsxSm9QRLSQpEVSd7/TYJUb49TX7gztpjjEffnoVw66+Ytovs14Yp7HaKmUXeX9rKUoMoLNW3srqI5fWn8JejrVkK0QcrkFLOgS39yoKUQe292WJ1guUHG8K2o8K00oO1BTvXoW4yasclUTgZYJY9aFNfAThX5CZRmczAV52oAPoupHhWRIUUAOoyUIlYVaAa/VbLbyiZUiyFbjQFNwiZQSGl4IDy9sO5Wrty0QLKhdZPxmgGcDo8ejn+c/6eiK9poz15Kw7Dr/vN/z6W7q++091/AQYA5mZ8GYJ9K0AAAAAASUVORK5CYII= ") left top no-repeat;
        }
    a.up {
        background : url("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAmlJREFUeNpsU0toU0EUPfPysx/tTxuDH9SCWhUDooIbd7oRUUTMouqi2iIoCO6lceHWhegy4EJFinWjrlQUpVm0IIoFpVDEIthm0dpikpf3ZuZ6Z94nrXhhMjM3c8895977BBHB2PznK8WPtDgyWH5q77cPH8PpdXuhpQT4ifR9u5sfJb1bmw6VivahATDrxcRZ2njfoaMv+2j7mLDn93MPiNRMvGbL18L9IpF8h9/TN+EYkMffSiOXJ5+hkD+PdqcLpICWHOHc2CC+LEyA/K+cKQMnlQHJX8wqYG3MAJy88Wa4OLDvEqAEOpJd0LxHIMdHBziowSwVlF8D6QaicK01krw/JynwcKoEwZczewroTvZirlKJs5CqQ5CG8pb57FnJUA0LYCXMX5fibd+p8LWDDemcPZbzQyjvH+Ki1TlIciElA7ghwLKV4kRZstt2sANWRjYTAGzuP2hXZFpJ/GsxgGJ0ox1aoFWsDXyyxqCs26+ydmagFN/rRjymJ1898bzGzmQE0HCZpmk5A0RFIv8Pn0WYPsiu6t/Rsj6PauVTwffTSzGAGZhUG2F06hEc9ibS7OPMNp6ErYFlKavo7MkhmTqCxZ/jwzGA9Hx82H2BZSw1NTN9Gx8ycHkajU/7M+jInsDC7DiaEmo1bNl1AMr9ASFgqVu9MCTIzoGUimXVAnnaN0PdBBDCCYbEtMk6wkpQwIG0sn0PQIUF4GsTwLSIFKNqF6DVrQq+IWVrQDxAYQC/1SsYOI4pOxKZrfifiUSbDUisif7XlpGIPufXd/uvdvZm760M0no1FZcnrzUdjw7au3vu/BVgAFLXeuTxhTXVAAAAAElFTkSuQmCC ") left top no-repeat;
        }
    html[dir=rtl] a {
        font-family: Arial, Helvetica, sans-serif;
        background-position-x: center;
        }"
    #parentDirLinkBox {
        font-family: Arial, Helvetica, sans-serif;
        margin-bottom: 10px;
        padding-bottom: 10px;
        }
    #listingParsingErrorBox {
        font-family: Arial, Helvetica, sans-serif;
        border: 1px solid black;
        background: #fae691;
        padding: 10px;
        display: none;
        }
    .content {
        max-width: 500px;
        margin: auto;
        padding: 10px;
        }
</style>

</head>
<body>)=====";

const char script[] PROGMEM = R"=====(
<script>

function sortTable(l){
var e=document.getElementById("theader"),n=e.cells[l].dataset.order||"1",s=0-(n=parseInt(n,10));
e.cells[l].dataset.order=s;var t,a=document.getElementById("tbody"),r=a.rows,d=[];
for(t=0;t<r.length;t++)d.push(r[t]);
for(d.sort(function(e,t){
    var a=e.cells[l].dataset.value,r=t.cells[l].dataset.value;
    return l?(a=parseInt(a,10),(r=parseInt(r,10))<a?s:a<r?n:0):r<a?s:a<r?n:0
}),t=0;t<d.length;t++)a.appendChild(d[t])
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
</script>
)=====";

const char body[] PROGMEM = R"=====(
        <h1> Uploader</h1>
        <form action='fupload' id='fupload'>
        <input class='file-url' type='text' name='fupload' id = 'fupload'  size='60'  value=''>
        <button class='buttons' style='margin-left:50px' type='submit'  onclick = "progress()"  >Upload </button></form><br>
        <progress id="progressBar" value="0" max="100" style="width:300px;" ></progress>
)=====";


const char footer[] PROGMEM = R"=====(
    </body>
    </html>)=====";
