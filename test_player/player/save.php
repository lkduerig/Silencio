<?php
// array_splice toimii tässä !!!

// VARS
	$feedback = "";
	
// FUNCTIONS
	function cleanInput($input_text) {
		$chars_to_remove = array("<", ">", "\n", "\r");
		$input_text = str_replace($chars_to_remove, "", $input_text);
		$input_text = str_replace("&", "ja", $input_text);
		$input_text = str_replace("\"", "'", $input_text);
		return $input_text;
	}

	function getNewEDL($EDLpath) {
		$debug = "";
		
		global $feedback;
		if($_POST['tyyppi'] == 'add') $feedback = "Virheilmoituksen tallennus epäonnistui!";
		if($_POST['tyyppi'] == 'edit') $feedback = "Virheilmoituksen tallennus epäonnistui!";
		if($_POST['tyyppi'] == 'remove') $feedback = "Virheilmoituksen poistaminen epäonnistui!";
		$my_raita = $_POST['raita'];
		$my_aika = 44100*$_POST['aika'];
		$my_aika_vanha = 44100*$_POST['aika_vanha'];
		$my_teksti = cleanInput(utf8_decode($_POST['teksti']));
		
		$edl_lines = file($EDLpath);
		$edl_key = 0;
		for($i = count($edl_lines)-1; $i>0 ; $i--) {
			if(strpos($edl_lines[$i], "---") == 1) {
				$edl_key = $i+1;
				break;
			}
		}
		
		$track_start = 0;
		$current_track = 0;
		$edl_key_for_new = 0;
		$marker_edit_deleted = false;
		for($i = $edl_key; $i < count($edl_lines); $i++) {
			if(substr($edl_lines[$i], 19, 1) == "1") {
				$track_start = floor(trim(substr($edl_lines[$i], 0, 12)));
				$current_track++;
				if($current_track == $my_raita) {
					$edl_key_for_new = $i+1;
					$my_aika += $track_start;
					$my_aika_vanha += $track_start;
				}
			}
			else if(substr($edl_lines[$i], 19, 1) == "0" && $current_track == $my_raita) {
				if($_POST['tyyppi'] != 'remove' && $my_aika > floor(trim(substr($edl_lines[$i], 0, 12)))) $edl_key_for_new = $i+1;
				if($_POST['tyyppi'] == 'remove' && $my_aika_vanha == floor(trim(substr($edl_lines[$i], 0, 12)))) {
					unset($edl_lines[$i]);
					$feedback = "Virheilmoitus poistettu!";
					continue;
				}
				if($_POST['tyyppi'] == 'edit' && $my_aika_vanha == floor(trim(substr($edl_lines[$i], 0, 12)))) {
					unset($edl_lines[$i]);
					$marker_edit_deleted = true;
					if($my_aika_vanha < $my_aika) {
						$edl_key_for_new--;
					}
					$feedback = "Muutokset tallennettu!";
					continue;
				}
				else if($_POST['tyyppi'] == 'edit') {
					if($my_aika_vanha < $my_aika && $marker_edit_deleted) {
						$edl_key_for_new--;
						$marker_edit_deleted = false;
					}
				}
			}
		}
		
		if($_POST['tyyppi'] != 'remove') {
			//if($_POST['tyyppi'] == 'add') $edl_key_for_new++;
			//else if($my_aika_vanha > $my_aika) $edl_key_for_new++;
			$my_aika = "".$my_aika;
			$my_aika_spaces = 12 - strlen($my_aika);
			for($i = 1; $i <= $my_aika_spaces; $i++) {
				$my_aika = " ".$my_aika;
			}
			$edl_new_line = $my_aika."       0                      \"".$my_teksti."\"\r\n";
			
			$edl_end = array_splice($edl_lines, $edl_key_for_new);
			array_push($edl_lines, $edl_new_line);
			$edl_lines_new = array_merge($edl_lines, $edl_end);
			if($_POST['tyyppi'] == 'add') $feedback = "Virheilmoitus tallennettu!";
		}
		else $edl_lines_new = $edl_lines;
		
		$reconstruct = "";
		foreach($edl_lines_new as $value) {
			$reconstruct .= $value;
		}
		
		//$feedback = $debug;
		return $reconstruct;
	}
	
	function writeNewEDL() {
		$data = getNewEDL(utf8_decode($_POST['edl']));
		$file = utf8_decode($_POST['edl']);
		$handle = fopen($file, 'w');
		fwrite($handle, $data);
		fclose($handle); 
	}
	
	if(!empty($_POST)) {
		writeNewEDL();
		echo "feedback=".$feedback;
	}
	else echo "feedback=Tietoja ei saatu tallennettua!";
	?>
