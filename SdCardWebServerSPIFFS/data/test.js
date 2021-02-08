function _(el) {
  return document.getElementById(el);
}



var xmlhttp = new XMLHttpRequest();
xmlhttp.onreadystatechange = function () {
  if (this.readyState == 4 && this.status == 200) {
    myFunction(this.responseText);
  }
}
xmlhttp.open("GET", "/list", true);
xmlhttp.send();

function getHomeDir() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      myFunction(this.responseText);
    }
  }
  xhttp.open("GET", "/home", true);
  xhttp.send();
}


function getBackDir() {
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
  var out = "<table class = \"center\">";
  out += "<tr><th>index</th><th>name</th><th>size</th><th>Delete file</th><th>Share file</th><tr>";
  for (i = 0; i < arr.length; i++) {
    var n = arr[i].name.lastIndexOf("/");
    var m = (arr[i].name.length - n);
    var name = arr[i].name.slice(-m);
    console.log(name);
    out += "<tr><td>" +
      arr[i].index +
      "</td><td>" +
      "<a href='/openFile?=" +
      arr[i].name +
      "'\">" +
      name +
      "</a></td><td>" +
      arr[i].size +
      "</td><td>" +
      "<button class='buttons' onclick=\"location.href='/deleteConfirm?file=" +
      arr[i].name +
      "';\">" +
      arr[i].Deletefile + "</button></td><td>" +
      "<button class='buttons' onclick = \"location.href ='/share?file=" + arr[i].name + "';\">" +
      arr[i].Sharefile + "</button></td></tr>";
  }
  out += "</table>";
  document.getElementById("fileTable").innerHTML = out;
}

setInterval(function () {
  // Call a function repetatively with 100 milli Second interval
  getData();
}, 100);

var percentage;

function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("percent").innerHTML =
        this.responseText + "%";
      percentage = this.responseText;
      document.getElementById("bar").style.width = this.responseText + '%';
      console.log(this.responseText);
    }
  };
  xhttp.open("GET", "readPer", true);
  xhttp.send();
}

