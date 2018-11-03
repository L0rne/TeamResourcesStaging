<?php
/**
 *  A CSS parser for PDFlib Clients
 *
 *  Modified version of this class: https://github.com/dbr/so_scripts/blob/master/csssparser.php
 *
 */
class triaxia_CSSParser {
    var $css;
    
    function __destruct() {
        unset($this->css);
    }
    
    function Clear() {
        unset($this->css);
        $this->css = array();
    }
    
    function Parse($filename) {
        $this->Clear();
        if(file_exists($filename)) {
            return $this->ParseStr(file_get_contents($filename));
        } else {
            return false;
        }
    }
    
    function ParseStr($str) {
        $this->Clear();
        // Remove comments
        $str = preg_replace("/\/\*(.*)?\*\//Usi", "", $str);
        
        $parts = preg_split("/}/", $str);
        //die(var_dump($parts));
        if(count($parts) > 0) {
            foreach($parts as $part) {
                if (preg_match("/[a-zA-Z]/", $part) < 1) continue; // don't bother, it's empty
                list($keystr,$codestr) = explode("{",$part); // preg_split('/\{/', $part, PREG_SPLIT_NO_EMPTY); //
                $keys = explode(",",trim($keystr)); //preg_split('/,/', trim($keystr), PREG_SPLIT_NO_EMPTY); // 
                if(count($keys) > 0) {
                    foreach($keys as $key) {
                        if(strlen($key) > 0) {
                            $key = str_replace("\n", "", $key);
                            $key = str_replace("\\", "", $key);
                            $this->Add($key, trim($codestr));
                        }
                    }
                }
            }
        }
        return (count($this->css) > 0);
    }
    
    function Add($key, $codestr) {
        $key = strtolower($key);
        $codestr = strtolower($codestr);
        if(!isset($this->css[$key])) {
            $this->css[$key] = array();
        }
        $codes = explode(";",$codestr);
        if(count($codes) > 0) {
            foreach($codes as $code) {
                $code = trim($code);
                if (preg_match("/[a-zA-Z]/", $code) < 1) continue; // don't bother, it's empty
                list($codekey, $codevalue) = explode(":",$code,2);
                if(strlen($codekey) > 0) {
                    $this->css[$key][trim($codekey)] = trim($codevalue);
                }
            }
        }
    }
    
    
    function Get($key, $property, $default = false) {
        // allows client to pass an array of hierarchically ordered selectors
        if (is_array($key)) {
            foreach ($key as $k) {
                $result = $this->Get($k, $property);
                if($result) return $result;
            }
            return $default;
        }
        
        // special handling for font-style and font-weight. PDFlib clients only understand
        // the css-equivalent of font-style. If CSS contains font-weight, concatenate.
        if ($property == 'font-style') {

            $bold = !isset($this->css[$key]['font-weight']) || $this->css[$key]['font-weight'] != 'bold' ? 0 : 1;
            $italic = !isset($this->css[$key]['font-style']) || $this->css[$key]['font-style'] != 'italic' ? 0 : 2;
            $fontstyles = array('normal', 'bold', 'italic', 'bolditalic');
            
            return $fontstyles[$bold+$italic];
        }
        
        if(!isset($this->css[$key][$property])) return $default;
        
        $value = $this->css[$key][$property];
        
        if (stristr($property, 'size') || stristr($property, 'margin')) {
            $value = preg_replace('/[^0-9|^-]/', '', $value);
        }
        
        if(stristr($property, 'padding')) {
            $value = preg_replace('/[a-zA-Z]/', '', $value);
        }
        
        if ($property == 'font-style' && isset($this->css[$key]['font-weight'])) {
            
        }
        
        return $value;
    }
    
    //private function hex_color_to_pdf_rgb($color) {
    //    if ($color[0] == '#')
    //        $color = substr($color, 1);
    //
    //    if (strlen($color) == 6)
    //        list($r, $g, $b) = array($color[0].$color[1],
    //                                 $color[2].$color[3],
    //                                 $color[4].$color[5]);
    //    elseif (strlen($color) == 3)
    //        list($r, $g, $b) = array($color[0].$color[0], $color[1].$color[1], $color[2].$color[2]);
    //    else
    //        return false;
    //
    //    $r = hexdec($r)/255; $g = hexdec($g)/255; $b = hexdec($b)/255;
    //
    //    return "rgb $r $g $b"; //array($r, $g, $b);
    //}
  
}