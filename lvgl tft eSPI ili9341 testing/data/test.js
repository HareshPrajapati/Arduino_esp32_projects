

function _(el) {
    return document.getElementById(el);
    }


  var xmlhttp = new XMLHttpRequest();
  xmlhttp.onreadystatechange=function() {
    if (this.readyState == 4 && this.status == 200) {
      myFunction(this.responseText);
    }
  }
  xmlhttp.open("GET", "/list", true);
  xmlhttp.send();



function getHomeDir(){
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      myFunction(this.responseText);
    }
  }
  xhttp.open("GET", "/home", true);
  xhttp.send();
}



function getBackDir(){
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      myFunction(this.responseText);
    }
  }
  xhttp.open("GET", "/previousDir", true);
  xhttp.send();
}


function myFunction(response) {
  var arr = JSON.parse(response);
  var i;
  var out ="<table>";

  out += "<tr><th>index</th><th>name</th><th>size</th><th>Delete file</th><th>Share file</th><tr>";

  for(i = 0; i < arr.length; i++) {
    out += "<tr><td>" +
    arr[i].index +
    "</td><td>" +
    "<a href='/readFile?=" +
    arr[i].name +
    "'\">"+
    arr[i].name +
    "</a></td><td>" +
    arr[i].size +
    "</td><td>"+
    "<button class='buttons' onclick=\"location.href='/deleteConfirm?file=" +
    arr[i].name+
    "';\">" +
    arr[i].Deletefile +"</button></td><td>"+
    "<button class='buttons'>"+
    arr[i].Sharefile +
    "</button></td></tr>";
  }
  out += "</table>";
  document.getElementById("fileTable").innerHTML = out;
}


function update() {
  var element = document.getElementById("myprogressBar");
  var width = 1;
  var identity = setInterval(scene, 50);
  function scene() {
    if (width >= 100) {
    clearInterval(identity);
    } else {
    width++;
    console.log(width);
    element.style.width = width + '%';
    element.innerHTML = width  + "%";
    }
  }
  }






