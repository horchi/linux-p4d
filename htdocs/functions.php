<?

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

?>
