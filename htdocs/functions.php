<?php

// ---------------------------------------------------------------------------
// Login
// ---------------------------------------------------------------------------

function haveLogin()
{
   if (isset($_SESSION['angemeldet']) && $_SESSION['angemeldet'])
      return true;

   return false;
}

function checkLogin($user, $passwd)
{
   $md5 = md5($passwd);

   if (requestAction("check-login", 5, 0, "$user:$md5", $resonse) == 0)
      return true;

   return false;
}

// ---------------------------------------------------------------------------
// Date Picker
// ---------------------------------------------------------------------------

function datePicker($title, $name, $year, $day, $month)
{
   $startyear = date("Y")-10;
   $endyear=date("Y")+1; 
   
   $months = array('','Januar','Februar','März','April','Mai',
                   'Juni','Juli','August', 'September','Oktober','November','Dezember');

   $html = $title . ": ";

   // day

   $html .= "  <select name=\"" . $name . "day\">\n";

   for ($i = 1; $i <= 31; $i++)
   {
      $sel = $i == $day  ? "SELECTED" : "";
      $html .= "     <option value='$i' " . $sel . ">$i</option>\n";
   }

   $html .= "  </select>\n";
   
   // month

   $html .= "  <select name=\"" . $name . "month\">\n";
   
   for ($i = 1; $i <= 12; $i++)
   {
      $sel = $i == $month ? "SELECTED" : "";
      $html .= "     <option value='$i' " . $sel . ">$months[$i]</option>\n";
   }

   $html.="  </select>\n";

   // year

   $html .= "  <select name=\"" . $name . "year\">\n";
   
   for ($i = $startyear; $i <= $endyear; $i++)
   {
      $sel = $i == $year  ? "SELECTED" : "";
      $html .= "     <option value='$i' " . $sel . ">$i</option>\n";
   }

   $html .= "  </select>\n";

   return $html;
}

// ---------------------------------------------------------------------------
// Check/Create Folder
// ---------------------------------------------------------------------------

function chkDir($path, $rights = 0777)
{ 
   if (!(is_dir($path) OR is_file($path) OR is_link($path)))
      return mkdir($path, $rights);
   else
      return true; 
}

// ---------------------------------------------------------------------------
// Request Action
// ---------------------------------------------------------------------------

function requestAction($cmd, $timeout, $address, $data, &$response)
{
   $timeout = time() + $timeout;
   $response = "";

   syslog(LOG_DEBUG, "p4: requesting ". $cmd . " with " . $address);

   mysql_query("insert into jobs set requestat = now(), state = 'P', command = '$cmd', address = '$address', data = '$data'")
      or die("Error" . mysql_error());
   $id = mysql_insert_id();

   while (time() < $timeout)
   {
      usleep(1000);

      $result = mysql_query("select * from jobs where id = $id and state = 'D'")
         or die("Error" . mysql_error());

      if (mysql_numrows($result))
      {
         $buffer = mysql_result($result, 0, "result");
         list($state, $response) = split(":", $buffer, 2);

         if ($state == "fail")
            return -2;

         return 0;
      }
   }

   syslog(LOG_DEBUG, "p4: timeout on " . $cmd);

   return -1;
}

?>
