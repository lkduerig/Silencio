<?php 
// VARS
	$kirjat = array();
	$cdt = array();
	$raidat = array();

// FUNCTIONS

	function getEDL($EDLpath, $EDLtype) {	
		$edl_lines = file($EDLpath);
		$edl_key = 0;
		for($i = count($edl_lines)-1; $i>0 ; $i--) {
			if(strpos($edl_lines[$i], "---") == 1) {
				$edl_key = $i+1;
				break;
			}
		}
		
		$prev_track_start = 0;
		$track_start = 0;
		$track_time = 0;
		$current_track = 0;
		$edl_carrier = array();
		
		if($EDLtype == 'markers') {
			for($i = $edl_key; $i < count($edl_lines); $i++) {
				if(substr($edl_lines[$i], 19, 1) == "1") {
					$track_start = floor(trim(substr($edl_lines[$i], 0, 12))/44100);
					$current_track++;
				}
				else if(substr($edl_lines[$i], 19, 1) == "0") {
					$edl_carrier[$current_track][floor(trim(substr($edl_lines[$i], 0, 12))/44100)-$track_start] = utf8_encode(substr($edl_lines[$i], 43, (strlen($edl_lines[$i])-46)));
				}
			}
		}
		else if($EDLtype == 'times') {
			for($i = $edl_key; $i < count($edl_lines); $i++) {
				if(substr($edl_lines[$i], 19, 1) == "1" || substr($edl_lines[$i], 19, 1) == "4") {
					$track_time = floor(trim(substr($edl_lines[$i], 0, 12))/44100) - $prev_track_start;
					$prev_track_start = floor(trim(substr($edl_lines[$i], 0, 12))/44100);
					$edl_carrier[$current_track] = $track_time;
					$current_track++;
				}
			}
		}
		return $edl_carrier;
	}

// GET FILE HIERARCHY
	$markers_book = 0;
	$markers_cd = 0;
	$duration_book = 0;
	$duration_cd = 0;
	foreach(scandir('.') as $kirja) {
		if(!is_dir($kirja) || $kirja[0] == '.') continue;
		
		foreach(scandir($kirja) as $cd) {
			if(!is_dir($kirja.'/'.$cd) || $cd[0] == '.') continue;
			
			foreach(scandir($kirja.'/'.$cd) as $raita) {
				if(!is_dir($kirja.'/'.$cd.'/'.$raita) && substr($raita, strrpos($raita, '.') + 1) == 'EDL') {
					$raidat['EDL'] = $kirja.'/'.$cd.'/'.$raita;
					$edl_info = getEDL($raidat['EDL'], 'markers');
					$edl_durations = getEDL($raidat['EDL'], 'times');
					if(!empty($edl_info)) {
						foreach($edl_info as $value) $markers_cd += count($value);
						$markers_book += $markers_cd;
					}
					if(!empty($edl_durations)) {
						foreach($edl_durations as $value) $duration_cd += $value;
						$duration_book += $duration_cd;
					}
				}
				if(is_dir($kirja.'/'.$cd.'/'.$raita) || substr($raita, strrpos($raita, '.') + 1) != 'mp3') continue;
				array_push($raidat, $raita);
			}
			
			$raidat['markers'] = $markers_cd;
			$raidat['duration'] = $duration_cd;
			$cdt[$cd] = $raidat;
			$raidat = array();
			$markers_cd = 0;
			$duration_cd = 0;
			
		}
		
		$cdt['markers'] = $markers_book;
		$cdt['duration'] = $duration_book;
		$kirjat[$kirja] = $cdt;
		$cdt = array();
		$markers_book = 0;
		$duration_book = 0;
	}
	//print_r($kirjat);
	
// TEST EDL

	//print_r(getEDL('runoa!/cd1/runoa!_lopullinen_korjaus_ja_uudelleen_järj.EDL'));
	
// BUILD XML

	header('Content-Type: text/xml; charset=utf-8');
	echo "<?xml version='1.0' encoding='utf-8'?>\n";
	echo "<hierarchy>\n";
	foreach($kirjat as $kirja => $cdt) {
		$markers = 0;
		$duration = 0;
		if(array_key_exists('markers', $cdt)) {
			$markers = $cdt['markers'];
			unset($cdt['markers']);
		}
		if(array_key_exists('duration', $cdt)) {
			$duration = $cdt['duration'];
			unset($cdt['duration']);
		}
			
		echo "\t<book name='".utf8_encode($kirja)."' markers='".$markers."' duration='".$duration."'>\n";
		
		foreach($cdt as $cd => $raidat) {
			$edl = "";
			$edl_info = array();
			$edl_times = array();
			if(array_key_exists('EDL',$raidat)) {
				$edl = $raidat['EDL'];
				$edl_info = getEDL($edl, 'markers');
				$edl_times = getEDL($edl, 'times');
				unset($raidat['EDL']);
			}
			$markers = 0;
			$duration = 0;
			if(array_key_exists('markers',$raidat)) {
				$markers = $raidat['markers'];
				unset($raidat['markers']);
			}
			if(array_key_exists('duration',$raidat)) {
				$duration = $raidat['duration'];
				unset($raidat['duration']);
			}
			echo "\t\t<cd name='".utf8_encode($cd)."' edl='".utf8_encode($edl)."' markers='".$markers."' duration='".$duration."'>\n";
			
			$i = 1;
			foreach($raidat as $raita) {
				echo "\t\t\t<track name='".utf8_encode($raita)."'";
				if(!empty($edl_times) && array_key_exists($i, $edl_times)) echo " duration='".$edl_times[$i]."'";
				echo ">";
				if(!empty($edl_info) && array_key_exists($i, $edl_info)) {
					foreach($edl_info[$i] as $key => $value) echo "\n\t\t\t\t<marker time='".$key."'>".$value."</marker>";
					echo "\n";
				}
				echo "\t\t\t</track>\n";
				$i++;
			}
			echo "\t\t</cd>\n";
		}
		echo "\t</book>\n";
	}
	echo "</hierarchy>\n";

?>