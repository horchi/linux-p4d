/*
JavaScript-File for the p4d Web-InterFace
Copyright by Stefan Doering and Joerg Wendel
used for different functions in all pages
please do not modify unless you know what you are doing!
*/

function confirmSubmit(msg)
{
   if (confirm(msg))
      return true;

   return false;
} 

function showContent(elm){
		if (document.getElementById(elm).style.display == "block") {document.getElementById(elm).style.display = "none"} else {document.getElementById(elm).style.display = "block"}
}

function readonlyContent(elm, chk)
{
  var elm = document.querySelectorAll('[id*=' + elm + ']');  var i;
	if (chk.checked == 1){ 
		for (i = 0; i < elm.length; i++) {
			elm[i].readOnly = false;
			elm[i].style.backgroundColor = "#fff"; 
		}
	}else{
		for (i = 0; i < elm.length; i++) {
			elm[i].readOnly = true;
			elm[i].style.backgroundColor = "#ddd"; 
		} 
	}
}

function disableContent(elm, chk)
{
  var elm = document.querySelectorAll('[id*=' + elm + ']');  var i;
	if (chk.checked == 1){ 
		for (i = 0; i < elm.length; i++) {
			elm[i].disabled = false;
		}
	}else{
		for (i = 0; i < elm.length; i++) {
			elm[i].disabled = true;
		} 
	}
}
