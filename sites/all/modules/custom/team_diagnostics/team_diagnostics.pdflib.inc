<?php
// $Id$

/**
 * @file
 * Implements PDFlib functionality for team diagnostics
 *
 * TODO: 
 * 
 */

require_once(dirname(__FILE__) . '/team_diagnostics.css-parser.inc');
require_once(dirname(__FILE__) . '/team_diagnostics.data-manipulation.inc');
require_once(dirname(__FILE__) . '/team_diagnostics.pdflib-chart.inc');
define('TXP_PDFLIB_DEFAULT_FONT', 'cartogothicpro-book');

 class DiagnosticPDF {
    
    private $pdf_lib = 0;
    private $repeated = array();
    private $page_closed = false;
    private $report_info = array();
    private $page_num = 1;
    private $response_details = array();
    private $aggregate_data = array(); // contains arrays for category and survey_item averages (means)
    private $styles; // = array();
    // private $chart_real_path;
    private $doc_blocks = array();
    private $page_properties = array();
    private $dmo;
    private $y_cursor;
    private $last_textflow_header = array();
    
    function __construct($report_args = array(), $template_folder_uri = 'public://report_templates', /*$chart_image_folder_uri = 'public://pcharts', */ $outputfilename = '') {
        $creator = isset($report_args['creator']) ? $report_args['creator'] : 'Team Benchmark';
        $title = isset($report_args['create']) ? $report_args['create'] : 'Team Diagnostic';
        
        //TODO: Set all the margin properties from CSS based on "body", ".page", "{base_class}" selectors.
        $this->page_properties['margin_llx'] = 50; // = isset($report_args['margin_top'])
        $this->page_properties['margin_lly'] = 50;
        $this->page_properties['margin_urx'] = 562;
        $this->page_properties['margin_ury'] = 722;
        $this->page_properties['width'] = 612;
        $this->page_properties['height'] = 792;
        $this->page_properties['margin'] = array(
            'top'       => $this->page_properties['height'] - $this->page_properties['margin_ury'],
            'right'     => $this->page_properties['width'] - $this->page_properties['margin_urx'],
            'bottom'    => $this->page_properties['margin_lly'],
            'left'      => $this->page_properties['margin_llx']
        );
        
        $this->y_cursor = $this->page_properties['height'];
        
        $this->styles = $report_args['css'];
        unset($report_args['css']);
        
        $this->report_info = $report_args;
        if (!isset($this->report_info['base_class'])) $this->report_info['base_class'] = 'diagnostic';
        
        $this->dmo = new DiagnosticDMO($report_args['line_item_id'], $this->report_info['team_name']);
        $this->dmo->set_category_order($report_args['default_category_order']); //TODO: handle !isset(default_category_order)
        
        //$this->fetch_response_details($report_args['line_item_id']);
        
        
        $searchpath = drupal_realpath($template_folder_uri);
        // $this->chart_real_path = drupal_realpath($chart_image_folder_uri);
        $outfilename = '';
        
        
        try {
            $this->pdf_lib = new PDFlib();
            //dpm($_SERVER['HTTP_HOST'], "host");
            if ($_SERVER['HTTP_HOST'] == 'teamresources.com' || $_SERVER['HTTP_HOST'] == 'www.teamresources.com') {
               //$this->pdf_lib->set_option("license=L800602-010300-737438-NER8D2-PNDJ72");
                $this->pdf_lib->set_option("license=L900602-010300-741736-Y7P7B2-AGJGH2");
            }
            
            # This means we must check return values of load_font() etc.
            $this->pdf_lib->set_option("errorpolicy=return");
            $this->pdf_lib->set_option("textformat=bytes"); // NECESSARY for TABLES!!! (see PDFLib documentation)
            $this->pdf_lib->set_option("SearchPath=" . $searchpath);
        
            // this call to begin_document must be matched with a corresponding end_document!!
            if ($this->pdf_lib->begin_document($outfilename, "") == 0)
                drupal_set_message("Error: " . $this->pdf_lib->get_errmsg(), 'error', FALSE);
        
            $this->pdf_lib->set_info("Creator", $creator);
            $this->pdf_lib->set_info("Title", $title);
            
        }catch (PDFlibException $e) {
            $err_msg = "PDFlib exception occurred in DiagnosticPDF constructor:\n" .
                "[" . $e->get_errnum() . "] " . $e->get_apiname() . ": " .
                $e->get_errmsg() . "\n";
            
            drupal_set_message($err_msg, 'error', FALSE);
        }
        catch (Exception $e) {
            die($e);
        }
    }
    
    function __destruct() {
        if ($this->pdf_lib != 0) {
            try {
                // this matches the call to begin_document in the constructor
                $this->pdf_lib->end_document("");
            }catch (PDFlibException $e) {
                $err_msg = "PDFlib exception occurred in DiagnosticPDF destructor:\n" .
                    "[" . $e->get_errnum() . "] " . $e->get_apiname() . ": " .
                    $e->get_errmsg() . "\n";
                
                drupal_set_message($err_msg, 'error', FALSE);
            }
            catch (Exception $e) {
                die($e);
            }
            
            $this->pdf_lib = 0;
        }
    }
    
    /**
     *  $page is the page number of the template used to create this report,
     *  NOT the page number of the report itself. Here and elsewhere in the
     *  code, various functions need to know which page of the template is
     *  being cloned for dynamic content placement.
     *
    */
    function merge_template($template_filename) {
        //dpm($template_filename, "template_filename");
        try {
            
            $pdi_doc = $this->pdf_lib->open_pdi_document($template_filename, "");
            if ($pdi_doc == 0) {
                //set_drupal_message("Error: " . $this->pdf_lib->get_errmsg(), 'error', FALSE);
                //continue;
                die('Error: ' . $this->pdf_lib->get_errmsg());
            }
            
            $this->doc_blocks = $this->get_blocks($pdi_doc); // loads template blocks
            //die(var_dump($doc_blocks));
    
            $endpage = $this->pdf_lib->pcos_get_number($pdi_doc, "length:pages"); // gets last page of template
    
            /* Loop over all pages of the input document */
            for ($pageno = 1; $pageno <= $endpage; $pageno++) {

                // obnoxiously, the "page" parameter for open_pdi_page() is not zero-based.
                $page = $this->pdf_lib->open_pdi_page($pdi_doc, $pageno, "cloneboxes"); // opens a specific template page
                $this->page_closed = false;
    
                if ($page == 0) {
                    set_drupal_message("Error: " . $this->pdf_lib->get_errmsg(), 'error', FALSE);
                    continue;
                }
                
                /* Dummy $page size; will be adjusted later */
                $this->pdf_lib->begin_page_ext(10, 10, "");
                // Place the imported $page on the output $page, and adjust the $page size
                $this->pdf_lib->fit_pdi_page($page, 0, 0, "cloneboxes"); // "adjustpage");
                
                $page_blocks = $this->doc_blocks[$pageno]; // get blocks on this page of the template
                foreach ($page_blocks as $delta => $block) {
                    $this->fill_block($page, $block);
                }
                
                $this->pdf_lib->close_pdi_page($page);
                
                if(!$this->page_closed) { // is this necessary, given the line above?
                    
                    $this->do_repeated_blocks();
                    $this->pdf_lib->end_page_ext("");
                    $this->page_num++;
                }
                else { // is this ever NOT called?
                    die(var_dump($page));
                }
            }
            
            $this->pdf_lib->close_pdi_document($pdi_doc);
        
        }catch (PDFlibException $e) {
            $err_msg = "PDFlib exception occurred in DiagnosticPDF->merge_template():\n" .
                "[" . $e->get_errnum() . "] " . $e->get_apiname() . ": " .
                $e->get_errmsg() . "\n";
            
            drupal_set_message($err_msg, 'error', FALSE);
        }
        catch (Exception $e) {
            die($e);
        }
    }
    
    
    /**
     *  Helper function to create a user-friendlier array of blocks
     *  In addition to the PDFLib block properties...
     *  Block Properties: name, id, css_id, pdi_type, coords, width, height, repeated:boolean
     *
     */
    private function get_blocks($pdi_doc) {
        $blocks = array();
        
        $page_count = $this->pdf_lib->pcos_get_number($pdi_doc, "length:pages");
        
        if ($page_count < 1) {
            return $blocks;
        }
        
        /* Loop over all pages of the input document */
        for ($page_index = 0; $page_index < $page_count; $page_index++) {
            
            $page_number = $page_index + 1;
            
            $block_count = $this->pdf_lib->pcos_get_number($pdi_doc, 'length:pages['. $page_index .']/PieceInfo/PDFlib/Private/Blocks');
            //if ($block_count == 0) continue;
            
            $blocks[$page_number] = array();
            for ($block_index = 0; $block_index < $block_count; $block_index++) {
                $block = array();
                // get block name
                $block['name'] = $block['id'] = $this->pdf_lib->pcos_get_string($pdi_doc,
                    'pages[' . $page_index . ']/PieceInfo/PDFlib/Private/Blocks[' . $block_index . ']/Name');
                $block['css_id'] = '#' . preg_replace('/_/', '-', $block['id']);
                
                // get block PDI type
                $block['pdi_type'] = $this->pdf_lib->pcos_get_string($pdi_doc,
                    'pages[' . $page_index . ']/PieceInfo/PDFlib/Private/Blocks[' . $block_index . ']/Subtype');
                
                $is_textflow = $this->pdf_lib->pcos_get_string($pdi_doc, 'type:pages['. $page_index .']/blocks/'. $block['name'] .'/textflow') == 'boolean';
                if ($is_textflow) {
                    $block['pdi_type'] = 'textflow';
                }
                
                
                //if (stristr($block['pdi_type'], 'Text')) {
                //    // get font information
                //    $block['font_name'] = $this->pdf_lib->pcos_get_string($pdi_doc, 'pages['. $page_index .']/blocks/'. $block['name'] . '/fontname'); 
                //    $block['font_size'] = $this->pdf_lib->pcos_get_number($pdi_doc, 'pages['. $page_index .']/blocks/'. $block['name'] . '/fontsize'); 
                //    
                //    // get default content TODO: implement this globally
                //    if ($block['name'] == 'table_0_paragraph' )
                //        $block['default_text'] = $this->pdf_lib->pcos_get_string($pdi_doc, 'pages['. $page_index .']/blocks/'. $block['name'] . '/defaulttext'); 
                //
                //}
                $block = $this->append_block_css($block);
                
                
                // get block coordinates
                // $llx = $block['coords'][0]; $lly = $block['coords'][1]; $urx = $block['coords'][2]; $ury = $block['coords'][3];
                $coords = array();
                for ($j = 0; $j < 4; $j++) {
                    $coords[] = $this->pdf_lib->pcos_get_number($pdi_doc,
                        'pages[' . $page_index . ']/PieceInfo/PDFlib/Private/Blocks[' . $block_index . ']/Rect[' . $j . ']');
                }
                $block['coords'] = $coords;
                $x1 = $block['coords'][0];
                $y1 = $block['coords'][1];
                $x2 = $block['coords'][2];
                $y2 = $block['coords'][3];
                
                $block['width'] = abs($x2 - $x1);
                $block['height'] = abs($y2 - $y1);
                
                // get custom properties
                $custom_count = $this->pdf_lib->pcos_get_number($pdi_doc,
                    'length:pages[' . $page_index . ']/blocks[' . $block_index . ']/Custom');
                for ($custom_index = 0; $custom_index < $custom_count; $custom_index++) {
                    $key = $this->pdf_lib->pcos_get_string($pdi_doc,
                        'pages[' . $page_index . ']/PieceInfo/PDFlib/Private/Blocks[' . $block_index . ']/Custom[' . $custom_index . '].key');
                    
                    $block[$key] = $this->pdf_lib->pcos_get_string($pdi_doc,
                        'pages[' . $page_index . ']/PieceInfo/PDFlib/Private/Blocks[' . $block_index . ']/Custom[' . $custom_index . '].val');
                }
                
                if (isset($block['textflow_columns'])) $block['textflow_columns'] = explode(',', $block['textflow_columns']);
                
                // enforce "repeated" property
                if (!isset($block['repeated'])) {
                    $block['repeated'] = FALSE;
                } else {
                    if ($block['repeated'] == '0' || $block['repeated'] == 'false') {
                        $block['repeated'] = FALSE;
                    } else {
                        $block['repeated'] = TRUE;
                    }
                }
                
                $blocks[$page_number][$block['name']] = $block;
            
            } // block loop
            
        } // end page loop
        
        //$this->doc_blocks = $blocks;
        return $blocks;
        //die(var_dump($blocks));
    }
    
    private function append_block_css($block) {
        $selectors = array($block['css_id'], $this->report_info['base_class']);
        $block['font_name'] = $this->styles->Get($selectors, 'font-family', TXP_PDFLIB_DEFAULT_FONT);
        $block['font_size'] = $this->styles->Get($selectors, 'font-size', 10);
        
        return $block;
    }
    
    private function fill_block($page, $block, $block_args = array()) {
        
        global $base_url;
        
        // if this block is a repeated element, just add it to the repeated array
        // and let the set_repeated() function do the fill.
        if ($block['repeated']) {
            $this->repeated[] = $block;
            return;
        }
        
        //die(var_dump($block));
        $css_id = $block['css_id'];
        $font = $this->styles->Get($css_id, 'font-family', 'cartogothicpro-book');
        $font_size = $this->styles->Get($css_id, 'font-size', '12pt');
        $hex_color = $this->styles->Get($css_id, 'color', '#000000');
        $font_color = '{' . $this->hex_color_to_pdf_rgb($hex_color) . '}';
        if(!isset($block_args['options'])) {
            $block_args['options'] = "fontname=$font fontsize=$font_size encoding=unicode "; //fillcolor=$font_color"; //embedding";
        }
        
        $coord_string = implode('-', $block['coords']);
        $type = $block['type'];
        
        switch($type) {
            case 'chart':
                //$image_file = $base_url . another_sample_chart();
                //$this->fill_image($page, $block);
                $this->fill_chart($page, $block);
                break;
            case 'textflow':
            case 'textline':
                //$val = 'Block Coordinates: ' . $coord_string;
                $val = $this->get_data($block);
                if (isset($block['default_text'])) { $val = $block['default_text']; }
                $block_args['options'] = "fillcolor=$font_color";
                //die(var_dump(array($css_id, $block_args)));
                
                if ($this->pdf_lib->fill_textblock($page, $block['name'], $val, $block_args['options']) == 0) {
                   //drupal_set_message($this->pdf_lib->get_errmsg(), 'error', FALSE);
                   die('Error: ' . $this->pdf_lib->get_errmsg());
                }
                break;
            case 'table':
                $this->fill_table($page, $block);
                break;
            case 'dynamic_pdf':
                $this->fill_pdf($page, $block);
                break;
            case 'nested_blocks':
                $this->fill_data_view($page, $block);
                break;
            case 'data_view':
                $this->fill_data_view($page, $block);
                break;
            case 'replacement_string':
                $this->fill_replacement_string($page, $block);
                break;
        }
        
    }
    
    private function fill_pdf($page, $block) {
        
        $filename = $block['filename'];
        $pdf = $this->pdf_lib->open_pdi_document($filename, "");
        if ($pdf == 0)
            die("Error: " . $this->pdf_lib->get_apiname() . ": " . $this->pdf_lib->get_errmsg());
    
        /* Open the first page */
        $pdf_page = $this->pdf_lib->open_pdi_page($pdf, 1, "");
        if ($pdf_page == 0)
            die("Error: " . $this->pdf_lib->get_apiname() . ": " . $this->pdf_lib->get_errmsg());
        
        // instead of filling the PDF block, just place it using fit_pdi_page
        //if ($this->pdf_lib->fill_pdfblock($page, $block['name'], $pdf, "position={top left}") == 0) {
	//    die("Warning: " . $this->pdf_lib->get_errmsg());
	//}
        
        $llx = $block['coords'][0]; $lly = $block['coords'][1]; $urx = $block['coords'][2]; $ury = $block['coords'][3];
        
        //$ury = isset($block['block_above']) ? $this->doc_blocks[$page][$block['block_above']]['coords'][1] : $ury;
        if (isset($block['block_above'])) {
            // measure the bottom of the block above to the bottom margin of the page
            $block_above = $this->doc_blocks[$page][$block['block_above']];
            $remaining_space = $block_above['coords'][1] - $this->page_properties['margin_lly'];
            
            // if the PDF block won't fit into the remaining space on the page,
            // put it on the next one at the top of the page
            $block_height = $block['coords'][3] - $block['coords'][1] + 15;
            //$width = $block['coords'][2] - $block['coords'][0];
            if ($block_height > $remaining_space) {
                $this->start_new_page($page);
                
                $ury = $this->page_properties['margin_ury'] - 15;
            }
            else {
                $ury = $block_above['coords'][1];
            }
        }
        
        $this->pdf_lib->fit_pdi_page($pdf_page, $llx, $ury, "position={top left}");
        
    }
    
    /**
     *  This function fills data into the replacement_block.
     *
    */
    private function fill_replacement_string($page, $block) {
       
        // get data and string
        $str = $block['text'];
        //$str = "1.\tOverall, !strongest was rated the strongest characteristic for your team. So what becomes of the paragraph indentation if this text happens to be really long? I'm just checking for future reference. No other reason. You know how it is.";
        $replacement_keys = explode(',', $block['replacement_vars']);
        
        $replacement_values = $this->dmo->get_data($block);
        if (!is_array($replacement_values)) { $replacement_values = array($replacement_values); }
        $format_args = array_combine($replacement_keys, $replacement_values);
        $output = format_string($str, $format_args);
        $tab = "\t";
        $output = preg_replace('/\\\t/', $tab, $output); //preserve tab characters (note: literal "\t" requires triple escape \\\)
        
        // get CSS settings
        $selectors = array();
        if (isset($block['css_id'])) { $selectors[] = $block['css_id']; } //REFACTORED!
        $selectors[] = '.' . $this->report_info['base_class'] . ' .results-page-title';
        $selectors[] = '.results-page-title';
        
        $option_args = array();
        $option_args['font_name'] = $this->styles->Get($selectors, 'font-family', 'CartoGothicPro-Book');
        $option_args['font_size'] = $this->styles->Get($selectors, 'font-size', 10);
        $hex_color = $this->styles->Get($selectors, 'color', '#000000');
        $rgb_array = $this->hex_color_to_rgb_array($hex_color);
        $option_args['fillcolor'] = "rgb {$rgb_array['R']} {$rgb_array['G']} {$rgb_array['B']}";
        
        //krumo($output); krumo($page); exit();
        $options = $this->get_fittextline_options($selectors, $option_args);
        $this->pdf_lib->fill_textblock($page, $block['name'], $output, $options);
        
    }
    
    /**
     *  This function assumes a block in the main template contains a PDF filename
     *  to a PDF snippet with PDFLib blocks. The purpose of the blocks in this
     *  snippet is to provide presentation properties for a dataset.
     *
     *  The $master_block in the main template has a data key for a dataset from the
     *  DMO that iterates in a loop calling the configuration properties in the snippet's
     *  blocks to fit content into the report document.
     *
     *  In order for this to work, the block names in the snippet need to match the
     *  array keys in the dataset.
     *
     *  NOTE: the order of the blocks in the array representing the blocks in the snippet
     *  is not necessary the order they appear visually in the snippet. In order to maintain
     *  the desired order, the snippet must implement a "weight" property, lower weights
     *  will appear above higher weights. Height and Width will be taken from the block.
     *  For now, padding between the block is hard-coded. TODO: Use css to configure the
     *  space between the snippet blocks.
     *  
    */
    private function fill_data_view($page, $view_block) {        
        
        //die(var_dump($view_block));
        
        $view_data = $this->dmo->get_data($view_block);
        
        $field_names = explode(',', $view_block['fields']);
        $fields = array();
        foreach ($field_names as $field_name) {
            $fields[$field_name]['name'] = $field_name;
            $field_type = $view_block[$field_name . '_type'];
            $fields[$field_name]['type'] = $field_type;

            if (isset($view_block[$field_name . '_scale_max'])) {
                $fields[$field_name]['scale_max'] = $view_block[$field_name . '_scale_max'];
            }
            
            if (isset($view_block[$field_name . '_background_image'])) {
                $fields[$field_name]['background_image'] = $view_block[$field_name . '_background_image'];
                //die(var_dump($fields));
            }
        }
        //$coords = $master_block['coords'];
        $this->y_cursor = $new_page_y = $view_block['coords'][3];
        
        $bottom_margin = $this->styles->Get($view_block['class'], 'margin-bottom', 20);
        
        foreach ($view_data as $v_delta => $view) {
            foreach ($fields as $field) {
                $data = $view[$field['name']];
                $field['new_page_y'] = $new_page_y;
                if (is_array($data)) {
                    foreach ($data as $key => $datum) {
                        $output = $datum; // "#$key: $datum";
                        $this->fit_field($page, $output, $field);
                    }
                }
                else {
                    $this->fit_field($page, $data, $field);
                }
            }
            $this->y_cursor = $this->y_cursor - $bottom_margin;
        }
        
    }
    
    /**
     *  In some cases, rather than filling a block, we take the block's configuration
     *  properties and fit the PDF object directly into the document. This is necessary,
     *  for example, with tables and textflows where the height of the dynamic object
     *  is greater than the block's height. Filling the block would clip the content,
     *  so instead we fit the content on the page, adding new pages when necessary,
     *  and set this class's $this->y_cursor for subsequent content positioning.
     *
     *  IMPORTANT SEMANTIC DIFFERENCE: "fit" vs. "fill". They start with the same two
     *  letters and are only one character different in length, but they are completely
     *  different methods for getting content into a PDFLib document.
     *
     */
    private function fit_field($page, $data, $args = array()) {
        if (empty($args['coords'])) {
            $args['coords'] = array(
                0 => $this->page_properties['margin_llx'],
                1 => $this->y_cursor - 32, // arbitrary height of 1 in.
                2 => $this->page_properties['margin_urx'],
                3 => $this->y_cursor,
            );
        }
        
        //if ($args['type'] == 'mean_bar') die(var_dump($args));
        
        $coords = $args['coords'];
        
        $new_page_y = isset($args['new_page_y']) ? $args['new_page_y'] : $this->page_properties['margin_ury'];
            
        switch ($args['type']) {
            
            //if ($type == 'bullet_point') $datum_coord[0] = $datum_coord[0] + 30;
            case 'question_text':
                //die(var_dump($args));
                $selectors = array();
                $selectors[] = '#' . $this->report_info['base_class'] . ' .question-text';
                $selectors[] = '.' . $this->report_info['base_class'] . ' .question-text';
                $selectors[] = '.question-text';
                $args['font_name'] = $this->styles->Get($selectors, 'font-family', TXP_PDFLIB_DEFAULT_FONT);
                $args['font_size'] = $this->styles->Get($selectors, 'font-size', 10);
                
                $args['coords'][0] = $this->styles->Get($selectors, 'margin-left', $args['coords'][0]);
                $bottom_margin = $this->styles->Get($selectors, 'margin-bottom', 25);
                $right_margin = $this->page_properties['width'] - $args['coords'][2];
                $right_margin = $this->styles->Get($selectors, 'margin-right', $right_margin);
                $args['coords'][2] = $this->page_properties['width'] - $right_margin;
                $args['coords'][3] = $this->y_cursor - $this->styles->Get($selectors, 'margin-top', 1);
                
                // duplicate the same logic for a new page in fit_textflow_block. TODO: Move to a separate function.
                $four_lines = $args['font_size'] * 6;
                if ($args['coords'][3] - $this->page_properties['margin_lly'] < $four_lines) {
                    //start a new page
                    $this->start_new_page($page);
                    $this->y_cursor = $new_page_y; // $this->page_properties['margin_ury'];
                    $args['coords'][3] = $this->y_cursor; // = $this->page_properties['height'] - $this->page_properties['margin']['top']; // 792 - 36;
                    //die(var_dump($args));
                }
                
                //die(var_dump($args));
                $this->fit_textflow_block($page, $data, $args);
                $this->y_cursor = $this->y_cursor - $bottom_margin; //TODO: set this y padding descent from $args
                $args['bottom_margin'] = $bottom_margin;
                $this->last_textflow_header = array('page' => $page, 'data' => $data, 'args' => $args);
                
                break;
            case 'bulletpoint':
            case 'bullet_point':
                $selectors = array();
                $selectors[] = '#' . $this->report_info['base_class'] . ' .bullet-point';
                $selectors[] = '.' . $this->report_info['base_class'] . ' .bullet-point';
                $selectors[] = '.bullet-point';
                $args['font_name'] = $this->styles->Get($selectors, 'font-family', TXP_PDFLIB_DEFAULT_FONT);
                $args['font_size'] = $this->styles->Get($selectors, 'font-size', 10);
                
                // duplicate the same logic for a new page in fit_textflow_block. TODO: Move to a separate function.
                //$four_lines = $args['font_size'] * 3;
                //if ($args['coords'][3] - $this->page_properties['margin_lly'] < $four_lines) {
                //    //start a new page
                //    $this->start_new_page($page);
                //    $last_header = $this->last_textflow_header;
                //    $this->y_cursor = $this->page_properties['margin']['top'];
                //    $last_header['args']['coords'][3] = $this->y_cursor;
                //    $this->fit_textflow_block($last_header['page'], $last_header['data'], $last_header['args']);
                //    $args['coords'][3] = $this->y_cursor - $last_header['args']['bottom_margin']; // = $this->page_properties['height'] - $this->page_properties['margin']['top']; // 792 - 36;
                //}
                
                // duplicate logic from fit_textflow and question_text above. TODO: move all to a separate function.
                $four_lines = $args['font_size'] * 4;
                if ($args['coords'][3] - $this->page_properties['margin_lly'] < $four_lines) {
                    //start a new page
                    $this->start_new_page($page);
                    $last_header = $this->last_textflow_header;
                    $this->y_cursor = $last_header['args']['coords'][3] = $new_page_y; // $this->page_properties['margin_ury'];
                    $this->fit_textflow_block($last_header['page'], $last_header['data'], $last_header['args']);
                    $args['coords'][3] = $this->y_cursor;
                    $this->fit_textflow_block($page, 'Continued...', $args);
                    $args['coords'][3] = $this->y_cursor = $this->y_cursor - $last_header['args']['bottom_margin'];
                    //die(var_dump($args));
                }
                
                $x = $args['coords'][0];
                $y = $args['coords'][3];
                $bullet_img = $this->pdf_lib->load_image("auto", 'bullet.png', "");
                $optlist = "position={0 125} dpi={144 144}"; // scale={0.5}"; // dpi={144 144}";
                $this->pdf_lib->fit_image($bullet_img, $x, $y, $optlist);
                
                $args['coords'][0] = $this->styles->Get($selectors, 'margin-left', $args['coords'][0] + 30);
                $bottom_margin = $this->styles->Get($selectors, 'margin-bottom', 5);
                $right_margin = $this->page_properties['width'] - $args['coords'][2];
                $right_margin = $this->styles->Get($selectors, 'margin-right', $right_margin);
                $args['coords'][2] = $this->page_properties['width'] - $right_margin;
                $args['coords'][3] = $this->y_cursor - $this->styles->Get($selectors, 'margin-top', 1);
                
                //$args['coords'][0] = $args['coords'][0] + 30; //TODO: set padding between bullet image and text from $args or CSS
                
                $this->fit_textflow_block($page, $data, $args);
                $this->y_cursor = $this->y_cursor - $bottom_margin; //TODO: set this y padding descent from $args
                break;
            case 'mean_bar':
                $this->fit_mean_bar($data, $args);
                break;
            case 'category_label':
               // get CSS settings
               $selectors = array();
               $selectors[] = '#' . $this->report_info['base_class'] . ' .mean-bar-category';
               $selectors[] = '.' . $this->report_info['base_class'] . ' .mean-bar-category';
               $selectors[] = '.mean-bar-category';
               
               
               $x = $this->page_properties['margin']['left'] + $args['coords'][0]; //$left_margin; //$args['coords'][0];
               $y = $this->y_cursor;
               $top = $this->styles->Get($selectors, 'top', 0);
               $left = $this->styles->Get($selectors, 'left', 0);
               
               $font_name = $this->styles->Get($selectors, 'font-family', TXP_PDFLIB_DEFAULT_FONT);
               $font_size = $this->styles->Get($selectors, 'font-size', 10);
               
               $option_args = array(
                  'text_align' => 'center',
                  'font_name'     => $font_name,
                  'font_size'     => $font_size,
                  'width'         => 4 * $font_size,
                  'height'         => $font_size,
              );
              //die(var_dump($option_args));
              $text_opts = $this->get_fittextline_options($selectors, $option_args); //$pos_args, $pos_args);
              //die(var_dump($text_opts));
               $this->pdf_lib->fit_textline($data, $x + $left, $this->y_cursor - $top, $text_opts);
               break;
            default:
                $this->fit_textflow_block($page, $data, $args);
                $this->y_cursor = $this->y_cursor - 10; //TODO: set this y padding descent from $args
                break;
        }
    }
    
    private function fit_mean_bar($data, $args) {
        //die(var_dump(array('args' => $args, 'data' => $data)));
        
        // get CSS settings
        $selectors = array();
        $selectors[] = '#' . $this->report_info['base_class'] . ' .mean-bar';
        $selectors[] = '.' . $this->report_info['base_class'] . ' .mean-bar';
        $selectors[] = '.mean-bar';
        
        
        $stroke_color = $this->styles->Get($selectors, 'border-color', '#ccccff');
        $fill_color = $this->styles->Get($selectors, 'background-color', '#0000ff');
        $stroke_rgb = $this->hex_color_to_rgb_array($stroke_color);
        $fill_rgb = $this->hex_color_to_rgb_array($fill_color);
        $stroke_rgb = $fill_rgb = $this->get_data_color($data);
        
        $line_width = $this->styles->Get($selectors, 'border-width', 3);
        $radius = $this->styles->Get($selectors, 'border-radius', 0.1);
        $width = $this->styles->Get($selectors, 'width', $this->page_properties['margin_urx'] - $this->page_properties['margin_llx']);
        $height = $this->styles->Get($selectors, 'height', 16);
        $margin_bottom = $this->styles->Get($selectors, 'margin-bottom', 2);
        
        // get and set coordinates, size, and shape properties
        $left_margin = $this->styles->Get($selectors, 'margin-left', 1);
        $x = $this->page_properties['margin']['left'] + $left_margin; //$args['coords'][0];
        $y = $this->y_cursor;
        $scale_max = isset($args['scale_max']) ? (float)$args['scale_max'] : 5;
        //$scale_max = (float)$scale_max;
        // ASSUMPTION: globally, x-scale begins at 1
        $bar_width = $width * (($data - 1) / ($scale_max - 1));
        
        // draw rectangle
        $this->pdf_lib->setlinewidth($line_width);
        $this->pdf_lib->setcolor("stroke", "rgb", $stroke_rgb['R'], $stroke_rgb['G'], $stroke_rgb['B'], 0.0);
        $this->pdf_lib->setcolor("fill", "rgb", $fill_rgb['R'], $fill_rgb['G'], $fill_rgb['B'], 0.0);        
        
        //$this->pdf_lib->rect($x, $y, $bar_width, $height);
        /* Define a path for a rectangle with corners rounded by a given radius.
        * Start from the lower left corner and proceed counterclockwise.
        */
        $this->pdf_lib->moveto($x + $radius, $y);
        /* Start of the arc segment in the lower right corner */
        $this->pdf_lib->lineto($x + $bar_width - $radius, $y);
        /* Arc segment in the lower right corner */
        $this->pdf_lib->arc($x + $bar_width - $radius, $y + $radius, $radius, 270, 360);
        /* Start of the arc segment in the upper right corner */
        $this->pdf_lib->lineto($x + $bar_width, $y + $height - $radius );
        /* Arc segment in the upper right corner */
        $this->pdf_lib->arc($x + $bar_width - $radius, $y + $height - $radius, $radius, 0, 90);
        /* Start of the arc segment in the upper left corner */
        $this->pdf_lib->lineto($x + $radius, $y + $height);
        /* Arc segment in the upper left corner */
        $this->pdf_lib->arc($x + $radius, $y + $height - $radius, $radius, 90, 180);
        /* Start of the arc segment in the lower left corner */
        $this->pdf_lib->lineto($x , $y + $radius);
        /* Arc segment in the lower left corner */
        $this->pdf_lib->arc($x + $radius, $y + $radius, $radius, 180, 270);
        
        $this->pdf_lib->fill_stroke();
        //$this->pdf_lib->fill();
        
        // fit data label
        $font_name = $this->styles->Get($selectors, 'font-family', 'cartogothicpro-book'); //'CartoGothicPro-Book');
        $font_size = $this->styles->Get($selectors, 'font-size', '8pt');
        $font_color = $this->styles->Get($selectors, 'color', '#000000');
        $font_rgb = $this->hex_color_to_rgb_array($font_color);
        $padding_right = $this->styles->Get($selectors, 'padding-left', 5);
        $padding_bottom = $this->styles->Get($selectors, 'padding-bottom', 5);
        $this->pdf_lib->setcolor("fillstroke", "rgb", $font_rgb['R'], $font_rgb['G'], $font_rgb['B'], 0.0);
        $option_args = array(
            'text_align' => 'center',
            'font_name'     => $font_name,
            'font_size'     => $font_size,
            'width'         => 4 * $font_size,
            'height'         => $font_size,
        );
        //die(var_dump($option_args));
        $text_opts = $this->get_fittextline_options($selectors, $option_args); //$pos_args, $pos_args);
        //die(var_dump($text_opts));
        
        $mean = number_format($data, 2);
        $this->pdf_lib->fit_textline($mean, $x + $bar_width + $padding_right, $this->y_cursor + $padding_bottom, $text_opts);
        //$this->pdf_lib->fit_textline('(testing)', $x + $width + $padding_right, $this->y_cursor - $height - $padding_bottom, $text_opts);
        $this->pdf_lib->setcolor("fillstroke", "rgb", 0, 0, 0, 0.0);
        
        // insert background x-scale image
        $chart_background = isset($args['background_image']) ? $args['background_image'] : 'mean-bar-x-scale.pdf';
        $pdf = $this->pdf_lib->open_pdi_document($chart_background, "");
        if ($pdf == 0)
            die("Error: " . $this->pdf_lib->get_apiname() . ": " . $this->pdf_lib->get_errmsg());
    
        /* Open the first page */
        $pdf_page = $this->pdf_lib->open_pdi_page($pdf, 1, "");
        if ($pdf_page == 0)
            die("Error: " . $this->pdf_lib->get_apiname() . ": " . $this->pdf_lib->get_errmsg());

        $this->pdf_lib->fit_pdi_page($pdf_page, $x - 14, $y + 11, "position={top left}");
        
        
        $this->y_cursor = $this->y_cursor - $height - $margin_bottom;
        
    }
    
    private function fit_textflow_block($page, $text, $args = array()) {
        $coords = $args['coords'];
        $llx = $coords[0];
        $urx = $coords[2];
        
        $ury = $coords[3]; // $this->y_cursor;
        $lly = $this->page_properties['margin_lly']; // let textflow fill to the bottom margin of the page.
                                                     // Obviously, this function will not work for TextFlows
                                                     // that intentionally stop short of the page's bottom margin.
                                                     // In that case, use Blocks with the fill method.
                                                     
        //die(var_dump($args));
        $font_name = isset($args['font_name']) ? $args['font_name'] : TXP_PDFLIB_DEFAULT_FONT;
        $font_size = isset($args['font_size']) ? $args['font_size'] : 12;
        
        //$font = $this->pdf_lib->load_font($font_name, "unicode", "");
        //if ($font < 1) die(var_dump(array('args' => $args, 'font_handle' => $font)));
        
        $optlist1 = "fontname=$font_name fontsize=$font_size encoding=unicode ";
                    //"fillcolor={gray 0} alignment=justify";
        
        // assumes at least 4 lines must be available between the current y cursor position and the page's
        // bottom margin to begin a new textflow...
        //$four_lines = $font_size * 4;
        //if ($ury - $this->page_properties['margin_lly'] < $four_lines) {
        //    //start a new page
        //    $this->start_new_page($page);
        //    $ury = $this->y_cursor = $this->page_properties['height'] - $this->page_properties['margin']['top']; // 792 - 36;
        //}
        
        
        $texflow = $this->pdf_lib->add_textflow(0, $text, $optlist1);
        
        //TODO: Set these options dynamically
        $optlist = "verticalalign=justify linespreadlimit=120% ";
        $result = $this->pdf_lib->fit_textflow($texflow, $llx, $lly, $urx, $ury, $optlist);
        
        while ($result != '_stop') {
            $this->start_new_page($page);
            $ury = $this->y_cursor = $this->page_properties['height'] - $this->page_properties['margin']['top']; // 792 - 36;
            $result = $this->pdf_lib->fit_textflow($texflow, $llx, $lly, $urx, $ury, $optlist);
        }
        
        $tf_info = $this->get_textflow_info($texflow);
        
        $ury = $this->y_cursor = $tf_info['y1']; 
        
        $this->pdf_lib->delete_textflow($texflow);
        

    }
    
    private function start_new_page($page) {
        $this->do_repeated_blocks();
        $this->pdf_lib->end_page_ext("");
        $this->page_num++;
        
        $this->pdf_lib->begin_page_ext(10, 10, "");
        $this->pdf_lib->fit_pdi_page($page, 0, 0, "cloneboxes"); // "adjustpage");
    }
    
    private function get_textflow_info($textflow) {
        $tf_info = array();
        $tf_info['boundingbox'] = $this->pdf_lib->info_textflow($textflow, 'boundingbox');
        $tf_info['boxlinecount'] = $this->pdf_lib->info_textflow($textflow, 'boxlinecount');
        $tf_info['fittext'] = $this->pdf_lib->info_textflow($textflow, 'fittext');
        $tf_info['textheight'] = $this->pdf_lib->info_textflow($textflow, 'textheight');
        $tf_info['textendx'] = $this->pdf_lib->info_textflow($textflow, 'textendx');
        $tf_info['textendy'] = $this->pdf_lib->info_textflow($textflow, 'textendy');
        $tf_info['x1'] = $this->pdf_lib->info_textflow($textflow, 'x1');
        $tf_info['y1'] = $this->pdf_lib->info_textflow($textflow, 'y1');
        $tf_info['x2'] = $this->pdf_lib->info_textflow($textflow, 'x2');
        $tf_info['y2'] = $this->pdf_lib->info_textflow($textflow, 'y2');
        $tf_info['x3'] = $this->pdf_lib->info_textflow($textflow, 'x3');
        $tf_info['y3'] = $this->pdf_lib->info_textflow($textflow, 'y3');
        $tf_info['x4'] = $this->pdf_lib->info_textflow($textflow, 'x4');
        $tf_info['y4'] = $this->pdf_lib->info_textflow($textflow, 'y4');
        
        return $tf_info;
    }
    
   /**
    *  This helper function pre-processes the footer arrays before PDFLib renders them
    */
   private function preprocess_footers($block_info, &$data) {
       $footer_row_index = 0;
       
       foreach ($data['footer'] as $key => $footer) {
           $image_row_key = $key . '_image_row';
           // add an image row before the footer row that calls it
           if (isset($block_info[$image_row_key])) {
               $images = explode(',', $block_info[$image_row_key]);
               list($first, $repeated, $last) = $images;
               $column_count = count($footer);
               $image_row = array();
               $cell_index = 0;
               foreach ($footer as $delta => $cell_data) {
                   $type = $cell_index > 0 ? 'image' : 'text';
                   $value = $cell_index > 0 ? $repeated : '';
                   $value = $cell_index == 1 ? $first : $value;
                   $value = $cell_index == ($column_count - 1) ? $last : $value;
                   
                   $image_row[] =  array(
                       'colspan' => 1,
                       'type' => $type,
                       'value' => $value,
                   );
                   
                   $cell_index++;
               }
               $new_image_footer = array($image_row_key => $image_row);
               array_splice($data['footer'], $footer_row_index, 0, $new_image_footer);
           }
           
           $footer_row_index++;
       }
       
       if ($key === 'team_category_questions_response_with_summaries') {
   
           $new_row = array(
               'row_label' => array(
                   'colspan' => count($data['rows'][1]) - 1,
                   'type' => 'text',
                   'value' => 'Summary Score:',
                   'text-align' => 'right',
               ),
               'summary' => $footer['row_sum_response_sum_mean'],
           );
           $data['footer'][$key] = $new_row;
           
       }
   }

    
    private function fill_table($page, $block) {
        
        //die(var_dump($block));
        
        $table_data_args = array();
        
        // get table configuration from block
        if(!isset($block['headers'])) {
            die('headers not configured for table.');
        }
        $headers = explode(',', $block['headers']);
        
        if(!isset($block['item_data_key'])) {
            die('item_data_key not configured for table');
        }
       // $item_data = $this->get_data($block['item_data_key']); 
        $table_data_args['data_key'] = $block['item_data_key'];
        if(isset($block['category_filter'])) {
            $table_data_args['category_filter'] = $block['category_filter'];
        }
        
        $footer_data = array();
        if (isset($block['footer_data_key'])) {
            //$footer_data = $this->get_data($block['footer_data_key']);
            $table_data_args['footer_keys'] = explode(',', $block['footer_data_key']);
        }
        
        $table_data = $this->dmo->get_table($table_data_args);
        $this->preprocess_footers($block, $table_data);
        $item_data = $table_data['rows'];
        $footer_data = $table_data['footer'];
        
        $conditional_styles = $this->get_conditional_styles($table_data); //$block['item_data_key'], $footer_data); //TODO: implement this for all data cells in table
        /*******************************************************/
        /*********Add rows to PDFLib Table Object***************/
        /*******************************************************/
        
        //if ($block['id'] == 'willingness_response_table') die(var_dump($table_data)); //TODO: Remove this line.
        
        $col_widths = $this->get_columns_widths($block);
        
        $tbl = 0; $row = 1; $col = 1;
        
        // render headers
        $opt_args = $block; // this replaces the line below for nth-child options
        //$optlist = $this->get_option_list('add_table_cell_header', $block);
        
        $header_row_count = 1;
        
        /**START Team Name Refactoring**/
         //TODO: Refactor the following lines to read styles from CSS file.
         $font = $this->pdf_lib->load_font("cartogothicpro-book", "unicode", "");
         if ($font == 0) {
             die("Error: " . $this->pdf_lib->get_errmsg());
         }
         $team_header_optlist = "fittextline={position=center font=" . $font . " fontsize=12} " .
                               "colspan=" . count($headers);
                               
         //$this->get_option_list('add_table_cell_header', $opt_args);
         $tbl = $this->pdf_lib->add_table_cell($tbl, $col, $row++, $this->get_data("team_name"), $team_header_optlist);
        if ($tbl == 0) {
            die("Error: " . $this->pdf_lib->get_errmsg());
         }
        
         /**END Team Name Refactoring**/
        
         foreach ($headers as $header) {
            $col_index = $col - 1;
            $opt_args['nth-child'] = $col;
            if ( $col == count($headers) ) {
               $opt_args['last-child'] = TRUE;
               //die(var_dump($opt_args));
            }
            $optlist = $this->get_option_list('add_table_cell_header', $opt_args);
            //$optlist .= " stroke={ {line=horother linewidth=0.3} }";
            $col_optlist = 'colwidth=' . $col_widths[$col_index] . ' ' . $optlist;
            
            $tbl = $this->pdf_lib->add_table_cell($tbl, $col, $row, $header, $col_optlist);
	    if ($tbl == 0) {
               //drupal_set_message("Error: " . $this->pdf_lib->get_errmsg(), 'error', FALSE);
               die("Error: " . $this->pdf_lib->get_errmsg());
	    }
            $col++; //$header_row_count = 1;
            
         }
         $header_row_count++;
       
        
         // render row items
         $opt_args = $block;
        
         $font_name = $block['font_name'];
         $font = $this->pdf_lib->load_font($font_name, "unicode", "");
         $font_size = $block['font_size'];
         if ($font == 0) {
            die("Error: " . $this->pdf_lib->get_errmsg());
         }
         foreach ($item_data as $delta => $data_row) {
            $row++; $col = 1;
            
            foreach ($data_row as $column_key => $cell_data) {
                
               if (isset($block['textflow_columns']) && in_array($col, $block['textflow_columns'] )) {
                  $tfoptlist = "charref fontname=$font_name encoding=unicode fontsize=$font_size ";
                  $tf = $this->pdf_lib->add_textflow(0, $cell_data, $tfoptlist);
                  if ($tf == 0) {
                     die("Error: " . $this->pdf_lib->get_errmsg());
                  }
                 
                  $opt_args['textflow'] = $tf;
                  $tfoptlist = $this->get_option_list('add_table_cell_textflow', $opt_args);
                  //$tfoptlist .= " textflow=" . $tf;
                  //krumo($tfoptlist);
                  $tbl = $this->pdf_lib->add_table_cell($tbl, $col, $row, "", $tfoptlist);
                  if ($tbl == 0) {
                     die("Error: " . $this->pdf_lib->get_errmsg());
                  }
               }
               else {
                  if (isset($conditional_styles['rows'][$delta][$column_key])) {
                     $conditional_selectors = array();
                     $conditional_selectors[] = $block['css_id'] . ' .' . $conditional_styles['rows'][$delta][$column_key];
                     $conditional_selectors[] = $this->report_info['base_class'] . ' .' . $conditional_styles['rows'][$delta][$column_key];
                     $conditional_selectors[] = '.' . $conditional_styles['rows'][$delta][$column_key];
                     $optlist = $this->get_add_table_cell_options($conditional_selectors);
                     //die(var_dump($conditional_styles));
                  }
                  else {
                     $opt_args['row_num'] = $row;
                     $opt_args['nth-child'] = $col;
                     $option_method = $row%2 == 0 ?  'add_table_cell_row' : 'add_table_cell_alternate_row';
                     $optlist = $this->get_option_list($option_method, $opt_args);
                  }
                    
                  //$opt_args['nth-child'] = $col;
                  //$optlist = $this->get_option_list('add_table_cell_row', $opt_args);
                  $tbl = $this->pdf_lib->add_table_cell($tbl, $col, $row, $cell_data, $optlist);
                  if ($tbl == 0) {
                     die("Error: " . $this->pdf_lib->get_errmsg());
                  }
               }
                
               $col++;
               
            }
            
        }
        
        // render footer
        $optlist = $this->get_option_list('add_table_cell_footer', $block);
        $footer_row_count = 0;
        foreach ($footer_data as $delta => $data_row) {
            $row++; $col = 1; $footer_row_count++;
            
            foreach ($data_row as $column_key => $cell_data) {
               $colspan = (is_array($cell_data) && isset($cell_data['colspan'])) ? $cell_data['colspan'] : 1;
               $value = (is_array($cell_data) && isset($cell_data['value'])) ? $cell_data['value'] : $cell_data;
               $text_align = (is_array($cell_data) && isset($cell_data['text-align'])) ? $cell_data['text-align'] : 'center';
                
               //TODO: fix selector logic. Which method is responsible for formatting the selector for CSS declarations?
               if (isset($conditional_styles['footer'][$delta][$column_key])) {
                  $conditional_selectors = array();
                  $conditional_selectors[] = $block['css_id'] . ' .' . $conditional_styles['footer'][$delta][$column_key];
                  $conditional_selectors[] = $this->report_info['base_class'] . ' .' . $conditional_styles['footer'][$delta][$column_key];
                  $conditional_selectors[] = '.' . $conditional_styles['footer'][$delta][$column_key];
                  $optlist = $this->get_add_table_cell_options($conditional_selectors);
                  //krumo($optlist);
               }
               else {
                  $opt_args = $block;
                  $opt_args['nth-child'] = $col;
                  if (is_array($cell_data) && isset($cell_data['text-align'])) {
                    $opt_args['text-align'] = $cell_data['text-align'];
                  }
                  $optlist = $this->get_option_list('add_table_cell_footer', $opt_args);
                  //die(var_dump($optlist));
               }
               $optlist .= " colspan=" . $colspan;
               
               if (is_array($cell_data) && isset($cell_data['type']) && $cell_data['type'] == 'image') {
                  $image = $this->pdf_lib->load_image("auto", $value, "");
                  if ($image == 0) {
                     die("Couldn't load $image: " . $p->get_errmsg());
                  }
                  
                  $optlist = "image=$image fitimage={orientate=north boxsize={ 15 15 }} rowheight=20";
                  $value = '';
               }
               
               $tbl = $this->pdf_lib->add_table_cell($tbl, $col, $row, $value, /*$cell_data,*/ $optlist);
               if ($tbl == 0) {
                  die("Error: " . $this->pdf_lib->get_errmsg());
               }
               $col += $colspan; //++;
               
            }
            
            // fill out each column before going on to next row...
            if (count($data_row) < count($headers)) {
               while ($col <= count($headers)) {
                  $optlist = $this->get_option_list('add_table_cell_footer', $block);
                  
                  $tbl = $this->pdf_lib->add_table_cell($tbl, $col, $row, '', $optlist);
                  if ($tbl == 0) {
                     die("Error: " . $this->pdf_lib->get_errmsg());
                  }
                  $col++;
               }
            }
        }
        
         /*******************************************************/
         /*****************Render Table on Page******************/
         /*******************************************************/
         //{area=roweven fillcolor={gray 0.9}}
         //rgb 0.4 0.58 0.69
         // you cannot fill a block with a table object, so just take the coordinates and fit it on the page
         $table_args = array(
            'header_row_count' => $header_row_count,
            'footer_row_count' => $footer_row_count,
            'css_id'           => $block['css_id'],
         );
         $optlist = $this->get_fit_table_options($table_args); //... was ($block);
        
         //$llx = $block['coords'][0]; $lly = $block['coords'][1];
         $llx = isset($block['expandable']) && $block['expandable'] != 'no' ? $this->page_properties['margin_llx'] : $block['coords'][0];
         $lly = isset($block['expandable']) && $block['expandable'] != 'no' ? $this->page_properties['margin_lly'] : $block['coords'][1]; 
         $urx = $block['coords'][2];
         $ury = $block['coords'][3]; //TODO: Configure this for dynamic_y placement.
         if (isset($block['dynamic_y']) && $block['dynamic_y'] == 'TRUE') {
            $ury = $this->y_cursor - 10;
         }
        
         $result = $this->pdf_lib->fit_table($tbl, $llx, $lly, $urx, $ury, $optlist);
         if ($result ==  "_error") {
            die("Couldn't place table:  " . $this->pdf_lib->get_errmsg());
         }
        
        
         $llx = $this->page_properties['margin_llx']; $lly = $this->page_properties['margin_lly'];
         $urx = $this->page_properties['margin_urx']; $ury = $this->page_properties['margin_ury'] - 12; 
         while ($result == "_boxfull") {
            
            //$this->do_repeated_blocks();
            //$this->pdf_lib->end_page_ext("");
            //$this->page_num++;
            //
            //$this->pdf_lib->begin_page_ext(10, 10, "");
            //$this->pdf_lib->fit_pdi_page($page, 0, 0, "cloneboxes"); // "adjustpage");
            
            // above replaced with...
            $this->start_new_page($page);
            
            $result = $this->pdf_lib->fit_table($tbl, $llx, $lly, $urx, $ury, $optlist);
            if ($result ==  "_error") {
               die("Couldn't place table:  " . $this->pdf_lib->get_errmsg());
            }
            
         }
        
        // Reset the block coordinates with the coordinates of the table.
        // This will come in handy if there is a block below the table that
        // needs to be repositioned as the table expands.
        //$table = array();
        $this->doc_blocks[$page][$block['name']]['coords'][0] = $this->pdf_lib->info_table($tbl, 'x1');
        $this->doc_blocks[$page][$block['name']]['coords'][1] = $this->pdf_lib->info_table($tbl, 'y1');
        $this->doc_blocks[$page][$block['name']]['coords'][2] = $this->pdf_lib->info_table($tbl, 'x3');
        $this->doc_blocks[$page][$block['name']]['coords'][3] = $this->pdf_lib->info_table($tbl, 'y3');
        
        $this->y_cursor = $this->pdf_lib->info_table($tbl, 'y1');
        
        
        
        $this->pdf_lib->delete_table($tbl, "");
        
    }
    
    private function get_conditional_styles($table_data) {
        $conditional_styles = array();
        
        //die(var_dump($table_data));
        
        foreach ($table_data['rows'] as $delta => $row) {
            if (isset($row['row_label'])) $conditional_styles['rows'][$delta]['row_label'] = 'row-label';
            if (isset($row['row_sum_sum'])) $conditional_styles['rows'][$delta]['row_sum_sum'] = 'row-summary';
            if (isset($row['row_sum_mean'])) $conditional_styles['rows'][$delta]['row_sum_mean'] = 'row-summary';
        }
        
        foreach ($table_data['footer'] as $delta => $row) {
            
            //TODO: Refactor the condition styles with an array of styles for each condition
            //if (isset($row['row_sum_response_sum_mean'])) $conditional_styles['footer'][$delta]['row_sum_response_sum_mean'] = 'footer-row-summary';
            //if (isset($row['row_sum_mean'])) $conditional_styles['footer'][$delta]['row_sum_mean'] = 'footer-row-summary';
            //if (isset($row['row_sum_sum'])) $conditional_styles['footer'][$delta]['row_sum_sum'] = 'footer-row-summary';
            
            if (!isset($row['row_label'])) continue;
            
            //$conditional_styles['footer'][$delta]['row_label'] = 'row-label';
            $row_copy = $row;
            $row_label = array_shift($row_copy);
            $row_label = (is_array($row_label)) ? $row_label['value'] : $row_label; //added to accommodate row arrays
            $rank = strtolower(t('Rank'));
            if (strtolower($row_label) == $rank) {
                asort($row_copy);
                $keys = array_keys($row_copy);
                $conditional_styles['footer'][$delta][$keys[0]] = 'highest';
                $last_index = count($keys) - 1;
                $conditional_styles['footer'][$delta][$keys[$last_index]] = 'lowest';           
            }
         }
         
         return $conditional_styles;
    }
    
    private function get_columns_widths($block) {
        
        //die(var_dump($block));
        
        $headers = explode(',', $block['headers']);
        $col_widths = array();

        $td_n = 1;
        $td_n_selector = $block['css_id'] . " td:nth-child(" . $td_n . ")";        
        while($td_n_width = $this->styles->Get($td_n_selector, 'width')) {
            $col_widths[$td_n-1] = $td_n_width;
            $td_n_selector = $block['css_id'] . " td:nth-child(" . ++$td_n . ")";
        }
        if (count($col_widths) == count($headers)) return $col_widths; //TODO: Try to recover if css declarations dont' match headers
        
        
        if (isset($block['column_widths'])) {
            $col_widths = explode(',', $block['column_widths']);
            return $col_widths;
        }
        
        
        $col_num = count($headers);
        for ($col_idx = 0; $col_idx < $col_num; $col_idx++) {
            $col_widths[$col_idx] = number_format(100 / $col_num, 0) . '%';
        }
        
        return $col_widths;
        
    }
    
    private function get_option_list($pdflib_type, $option_args = array()) {
        
        //$alternate_selectors = array(
        //    'header_class' => array('th', 'tr:first-child'),
        //    'row_class' => array('tr', 'td'),
        //    'alternate_row_class' => array('tr:nth-child(odd)'),
        //    'footer_class' => array('tr:last-child'),
        //);
        //$option_args['alternate_selectors'] = $alternate_selectors;
        $selectors = array();
        switch($pdflib_type) {
            case 'add_table_cell_header':
                if (isset($option_args['last-child']) && $option_args['last-child']) { $selectors[] = $option_args['css_id'] .  ' th:last-child'; }
                if (isset($option_args['header_class'])) $selectors[] = ' .' . $option_args['header_class'];
                $selectors[] = $option_args['css_id'] . ' th:nth-child(' . $option_args['nth-child'] . ')';
                
                $selectors[] = $option_args['css_id'] . ' .header-row';
                $selectors[] = '.' . $this->report_info['base_class'] . ' .header-row';
                $selectors[] = $option_args['css_id'] . ' th';
                $selectors[] = '.' . $this->report_info['base_class'] . ' th';
                if (isset($option_args['last-child']) && $option_args['last-child']) { $selectors[] = 'th:last-child'; }
                $selectors[] = 'th:nth-child(' . $option_args['nth-child'] . ')';
                $selectors[] = 'th';
                return $this->get_add_table_cell_options($selectors);
                break;
            case 'add_table_cell_row':
                $selectors[] = $option_args['css_id'] . ' td:nth-child(' . $option_args['nth-child'] . ')';
                if (isset($option_args['row_class'])) $selectors[] = ' .' . $option_args['row_class'];                
                $row_class = $option_args['row_num'] % 2 == 0 ? '.row' : '.alt-row';
                $selectors[] = $option_args['css_id'] . ' ' . $row_class;
                $selectors[] = $row_class;
                $selectors[] = $option_args['css_id'] . ' td';
                $selectors[] = '.' . $this->report_info['base_class'] . ' td:nth-child(' . $option_args['nth-child'] . ')';
                $selectors[] = '.' . $this->report_info['base_class'] . ' td';
                $selectors[] = 'td:nth-child(' . $option_args['nth-child'] . ')';
                $selectors[] = 'td';
                
                return $this->get_add_table_cell_options($selectors);
                break;
            case 'add_table_cell_alternate_row':
                if (isset($option_args['alternate_row_class'])) $selectors[] = ' .' . $option_args['alternate_row_class'];
                $selectors[] = 'td:nth-child(odd)';
                return $this->get_add_table_cell_options($selectors);
                break;
            case 'add_table_cell_textflow':
                if (isset($option_args['alternate_row_class'])) $selectors[] = ' .' . $option_args['alternate_row_class'];
                $selectors[] = 'td:nth-child(odd)';
                return $this->get_add_table_cell_options($selectors, $option_args);
                break;
            case 'add_table_cell_footer':
               if (isset($option_args['nth-child'])) $selectors[] = $option_args['css_id'] . ' .footer-row:nth-child(' . $option_args['nth-child'] . ')';
                if (isset($option_args['footer_class'])) $selectors[] = ' .' . $option_args['footer_class'];
                $selectors[] = $option_args['css_id'] . ' .footer-row';
                $selectors[] = '.' . $this->report_info['base_class'] . ' .footer-row';
                if (isset($option_args['nth-child'])) $selectors[] = '.footer-row:nth-child(' . $option_args['nth-child'] . ')';
                $selectors[] = '.footer-row';
                $selectors[] = 'tr:last-child';
                return $this->get_add_table_cell_options($selectors, $option_args);
                break;
            case 'fit_table':
                break;
            case 'fit_textline':
                break;
        }
    }
    
    private function get_fittextline_options($class = array(), $option_args = array()) {
        $font_name = isset($option_args['font_name']) ? $option_args['font_name'] : 'CartoGothicPro-Book';
        $font_size = isset($option_args['font_size']) ? $option_args['font_size'] : '12';
        $text_align = isset($option_args['text_align']) ? $option_args['text_align'] : 'left';
        $color = isset($option_args['color']) ? $option_args['color'] : 'rgb 0 0 0';
        //$width = $option_args['width']; $height = $option_args['height'];
        
        $text_opts = 'fontname={' . $font_name . '} encoding=unicode fontsize=' . $font_size . ' textformat=utf8';
        if (isset($option_args['fillcolor'])) {
            $text_opts .= ' fillcolor={' . $option_args['fillcolor'] . '}';
        }
        
        //if (strtolower($text_align) == 'right') {
        //    
        //    $text_opts .= ' boxsize={' . $width . ' ' . $height . '} position={center right}';
        //}
        //
        //if (strtolower($text_align) == 'center' || strtolower($text_align) == 'middle') {
        //    
        //    $text_opts .= ' boxsize={' . $width . ' ' . $height . '} position={center middle}';
        //}
        
        return $text_opts;
    }
    
    
    private function get_add_table_cell_options($class = array(), $option_args = array()) {
        
        //$class = isset($option_args[$type]) ? $option_args[$type] : $option_args['alternate_selectors'][$type][0];
        //TODO: Refactor this method so it calls a hierachy of CSS selectors.
        
        // get options from css style or use defaults
        $font_name = TXP_PDFLIB_DEFAULT_FONT; // isset($option_args['font_name']) ? $option_args['font_name'] : 'helvetica';
        $font_name = $this->styles->Get($class, 'font-family', $font_name);
        $font_size = $this->styles->Get($class, 'font-size', 10);
        $font_style = $this->styles->Get($class, 'font-style', 'normal');
        $font_color = $this->styles->Get($class, 'color', '#000000');
        $font_color = $this->hex_color_to_pdf_rgb($font_color);
        $bg_color = $this->styles->Get($class, 'background-color', '#ffffff');
        $bg_color = $this->hex_color_to_pdf_rgb($bg_color);
        $text_align = strtolower($this->styles->Get($class, 'text-align', 'center'));
        $text_align = isset($option_args['text-align']) ? $option_args['text-align'] : $text_align;
        $vertical_align = strtolower($this->styles->Get($class, 'vertical-align', 'center'));
        $vertical_align = $vertical_align == 'middle' ? 'center' : $vertical_align;
        
        $font_opts = 'fontstyle=' . $font_style;
        
        $font = $this->pdf_lib->load_font($font_name, "unicode", $font_opts);
        if ($font == 0) {
            die("Error: " . $this->pdf_lib->get_errmsg());
        }
        
        
        $margin_positions = array(0 => 'margin_top', 1 => 'margin_right', 2 => 'margin_bottom', 3 => 'margin_left');
        $margin_str = $this->styles->Get($class, 'padding', '0 0 0 0'); // Yes, padding = margin in this translation
        $margin_array = explode(' ', trim($margin_str));
        $margins = array();
        foreach ($margin_positions as $idx => $margin) {
            $margins[$margin] = isset($margin_array[$idx]) ? $margin_array[$idx] : $margin[0];
        }
        
        $optlist = isset($option_args['textflow']) ?
                   "textflow=" . $option_args['textflow'] :
                   "fittextline={font=$font fontsize=$font_size fillcolor={" . $font_color . "} position={" . $text_align . " " . $vertical_align . "}} ";
        $optlist .= " marginleft=" . $margins['margin_left'] . " marginright=" . $margins['margin_right'] .
                   " margintop=" . $margins['margin_top'] . " marginbottom=" . $margins['margin_bottom'] .
                   " matchbox={fillcolor={" . $bg_color . "}}"; // stroke={ {line=horother linewidth=0.3}}
        
        return $optlist;
        
    }
    
    /**
     *  Here is a typical option configuration for fit_table:
     *
     *  $optlist = "header=1 footer=1 fill={ {area=roweven fillcolor={" . $roweven_bgcolor .
     *  "}} {area=header fillcolor={" . $header_bgcolor . "}} {area=footer fillcolor={" . $footer_bgcolor . "}} } " .
     *  "stroke={ {line=frame linewidth=1.5} {line=other linewidth=0.0} } ";
    */
    private function get_fit_table_options($option_args = array()) {
        // Right now, we're just fixing a border and letting the table no if it has headers and footers.
        $table_selector = array();
        $table_selector[] = $option_args['css_id'];
        $table_selector[] = 'table';
        
        $header_rows = isset($option_args['header_row_count']) ? $option_args['header_row_count'] : 0;
        $footer_rows = isset($option_args['footer_row_count']) ? $option_args['footer_row_count'] : 0;
        $border = $this->styles->Get($table_selector, 'border', 'solid 0px #ffffff');
        $border_args = $this->css_border_to_pdf_line_stroke($border);
        
        $frame = ' stroke={ ' .
                     '{line=frame linewidth=' . $border_args[1] . ' strokecolor=' . $border_args[2] .
                        ' dasharray=' . $border_args[0] . ' } ' .
                     //'{line=hor1 linewidth=0.5 strokecolor={rgb 0 0 0}} ' .
                     //'{line=horother linewidth=0.1 strokecolor={rgb 0.737 0.737 0.753}} ' .
                     //'{line=vert1 linewidth=0.1 strokecolor={rgb 0 0 0}} ' .
                  '}';
        //krumo($frame);
        return "header=$header_rows footer=$footer_rows $frame"; // stroke={ {line=frame linewidth=1.5} {line=other linewidth=0.0} } ";

    }
    
    private function css_border_to_pdf_line_stroke($css_border) {
        $border_args = explode(' ', $css_border);
        //TODO: make this smarter to allow designers to put the border values in any order,
        // for now, assume line-shape width color
        $strokes = array(
            'solid'     => '{1 1}',
            'dashed'    => '{5 5}',
            'dotted'    => '{1 4}'
        );
        
        $border_args[0] = $strokes[$border_args[0]];
        $border_args[1] = preg_replace('/[^0-9]/', '', $border_args[1]);
        $border_args[2] = '{' . $this->hex_color_to_pdf_rgb($border_args[2]) . '}';
        
        return $border_args;
    }
    
    private function fill_chart($page, $block) {
        $magnification = isset($block['magnification']) ? $block['magnification'] : 2;
        $height = $block['height'];
        $block['height'] = $block['height'] * $magnification;
        $block['width'] = $block['width'] * $magnification;
        $dpi_res = 72 * $magnification;
        
        //die(var_dump($block));
        
        $image = $this->get_chart_image_handle($block);
        /* Fill the image block */
	//$optlist = "";
        $optlist = "position={top left} dpi={" . $dpi_res . " " . $dpi_res . "}";
	if ($this->pdf_lib->fill_imageblock($page, $block['name'], $image, $optlist) == 0)
	    drupal_set_message($this->pdf_lib->get_errmsg(), 'error', FALSE);
    }
    
    private function fit_chart_image_block($block, $coords, $data) {
        $magnification = isset($block['magnification']) ? $block['magnification'] : 1;
        $height = $block['height'];
        $block['height'] = $block['height'] * $magnification;
        $block['width'] = $block['width'] * $magnification;
        $dpi_res = 72 * $magnification;
        $image = $this->get_chart_image_handle($block, $data);
        $x = $coords[0];
        $y = $coords[3];
        /* Fit the image block */
	$optlist = "position={top left} dpi={" . $dpi_res . " " . $dpi_res . "}"; // scale={0.5}"; // dpi={144 144}";
	
        $this->pdf_lib->fit_image($image, $x, $y, $optlist);
        
        $this->y_cursor = $y - $height; //$this->pdf_lib->info_image($image, 'y1', $optlist);
        
        //die(var_dump(array($this->y_cursor, $block)));
    }
    
    private function get_chart_image_handle($block, $data = array()) {
        $chart_args = $block;
        $chart_args['image_folder_uri'] = 'public://report_templates';
        $chart_args['base_class'] = $this->report_info['base_class'];
        $chart_args['class'] = isset($block['class']) ? $block['class'] : 'diagnostic-image';
        $chart_args['css'] = $this->styles;
        
        //$chart_args['width'] = $chart_args['width']; // * 2;
        //$chart_args['height'] = $chart_args['height']; // * 2;
        
        //die(var_dump($this->dmo));
        
        //TODO: This needs to be refactored to handle different methods
        //      for getting a chart. Currently, we either get a chart
        //      based on a data key recognized by the DMO or simply passing
        //      a single value for a simple barchart.
        $chart;
        if (empty($data))   {
            $chart = PdfLibChartFactory::create_chart($chart_args, $this->dmo);
        }
        else {
            $chart = PdfLibChartFactory::get_single_bar($chart_args, $data);
        }
        
        $image = $this->pdf_lib->load_image("auto", $chart->get_absolute_path(), "");
        if ($image == 0) {
            drupal_set_message('Error: ' . $this->pdf_lib->get_errmsg(), 'error', FALSE);
            return;
        }
        
        return $image;
    }
    
    private function fill_image($page, $block) {
        
        //$image_path = drupal_realpath($imagefile);
        //$image_path = drupal_realpath('public://pcharts') . '/another_bar_chart.png';
        $image_path = 'pdflib_scatterplot.png'; // 'file:///.file/id=6562758.28477858'; // '/Users/marcporlier/Documents/drupal-sites/benchmark/sites/default/files/pchart/another_bar_chart.png';
        $image = $this->pdf_lib->load_image("auto", $image_path, "");
        if ($image == 0) {
            drupal_set_message('Error: ' . $this->pdf_lib->get_errmsg(), 'error', FALSE);
            return;
        }
        /* Fill the image block */
	$optlist = "";
	if ($this->pdf_lib->fill_imageblock($page, $block['name'], $image, $optlist) == 0)
	    drupal_set_message($this->pdf_lib->get_errmsg(), 'error', FALSE);
    }
    
    private function do_repeated_blocks() {
        foreach ($this->repeated as $delta => $repeated_block) {
            $this->fill_repeated($repeated_block);
        }
    }
    
    private function fill_repeated($block, $block_args = array()) {
        //TODO: create a SWITCH to handle different block types
        $key = $block['data_key'];
        $value = $this->get_data($block); //$key);
        
        $x1 = $block['coords'][0];
        $y1 = $block['coords'][1];
        $x2 = $block['coords'][2];
        $y2 = $block['coords'][3];
        
        $width = $x2 - $x1;
        $height = $y2 - $y1;
        
        $selectors[] = $block['css_id'];
        $font_name = TXP_PDFLIB_DEFAULT_FONT; // isset($option_args['font_name']) ? $option_args['font_name'] : 'helvetica';
        $font_name = $this->styles->Get($selectors, 'font-family', $font_name);
        $font_size = $this->styles->Get($selectors, 'font-size', 10);
        $font_color = $this->styles->Get($selectors, 'color', '#000000');
        $font_color = $this->hex_color_to_pdf_rgb($font_color);
        $text_align = $this->styles->Get($selectors, 'text-align', 'left');
        $text_align = $text_align == 'center' ? 'middle' : $text_align;
        $vertical_align = $this->styles->Get($selectors, 'vertical-align', 'center');
        $vertical_align = $vertical_align == 'middle' ? 'center' : $vertical_align;
        //$font_style = $this->styles->Get($class, 'font-style', 'normal');
        //$font_color = $this->styles->Get($class, 'color', '#000000');
        //$font_color = $this->hex_color_to_pdf_rgb($font_color);
        
        //$font_name = isset($block['font_name']) ? $block['font_name'] : 'DejaVuSerif';
        //$font_size = isset($block['font_size']) ? $block['font_size'] : '12';
        //$text_align = isset($block['text_align']) ? $block['text_align'] : 'left';
        
        $text_opts = 'fontname={' . $font_name . '} fillcolor={' . $font_color . '} encoding=unicode fontsize=' . $font_size . ' textformat=utf8'; // removed embedding
        
        
        //if (strtolower($text_align) == 'right') {
            //$text_opts .= ' boxsize={' . $width . ' ' . $height . '} position={middle right}';
            $text_opts .= ' boxsize={' . $width . ' ' . $height . '} position={' . $vertical_align .' ' . $text_align . '}';
        //}
        
        //if (strtolower($text_align) == 'center' || strtolower($text_align) == 'middle') {
        //    
        //    $text_opts .= ' boxsize={' . $width . ' ' . $height . '} position={' . $vertical_align . ' middle}';
        //}
        
        //die(var_dump($text_opts));    
        $this->pdf_lib->fit_textline($value, $x1, $y1, $text_opts);
        
    }

    function print_pdf() {
        // this is the end of the line, so close up the document
        $report_filename = $this->get_data('report_filename');
        
        try {
            // this matches the call to begin_document in the constructor
            $this->pdf_lib->end_document("");
        
            $buf = $this->pdf_lib->get_buffer();
            $len = strlen($buf);
        
            header("Content-type: application/pdf");
            header("Content-Length: $len");
            header("Content-Disposition: inline; filename=\"{$report_filename}\"");
            print $buf;
        
        }catch (PDFlibException $e) {
            $err_msg = "PDFlib exception occurred in DiagnosticPDF->print_pdf():\n" .
                "[" . $e->get_errnum() . "] " . $e->get_apiname() . ": " .
                $e->get_errmsg() . "\n";
            
            drupal_set_message($err_msg, 'error', FALSE);
        }
        catch (Exception $e) {
            die($e);
        }
        
        $this->pdf_lib = 0;

    }
    
    /**
     *  Returns an array of the following structure:
     *
     *  $rgb_array = array(
     *      'R' => 1,
     *      'G' => 1,
     *      'B' => 1,
     *  );
     *
     *  Note: (The above array represents white. 1 is the max number for a color.)
     *  
     */
    private function hex_color_to_rgb_array($color) {
        if ($color[0] == '#')
            $color = substr($color, 1);
    
        if (strlen($color) == 6)
            list($r, $g, $b) = array($color[0].$color[1],
                                     $color[2].$color[3],
                                     $color[4].$color[5]);
        elseif (strlen($color) == 3)
            list($r, $g, $b) = array($color[0].$color[0], $color[1].$color[1], $color[2].$color[2]);
        else
            return false;
    
        $r = hexdec($r)/255; $g = hexdec($g)/255; $b = hexdec($b)/255;
        
        return array('R' => $r, 'G' => $g, 'B' => $b,);
    }
    
    private function hex_color_to_pdf_rgb($color) {
        $rgb = $this->hex_color_to_rgb_array($color);
        $r = $rgb['R'];
        $g = $rgb['G'];
        $b = $rgb['B'];
        return "rgb $r $g $b"; //array($r, $g, $b);
    }
    
    private function get_data_color($data_value) {
      $hex_color = '#aaaaaa';
      
      $precise_thresholds = array();
      
      foreach ($this->report_info['data_color_thresholds'] as $delta => $threshold) {
         if ($delta == 0) {
            $precise_thresholds[$delta]['min'] = (float)$threshold['min'] - .1;
            $precise_thresholds[$delta]['max'] = (float)$threshold['max'];
            $precise_thresholds[$delta]['color'] = $threshold['color'];
         }
         else {
            $precise_thresholds[$delta]['min'] = (float)$precise_thresholds[$delta-1]['max'];
            $precise_thresholds[$delta]['max'] = (float)$threshold['max'];
            $precise_thresholds[$delta]['color'] = $threshold['color'];
         }
      }
      
      foreach ($precise_thresholds as $delta => $threshold) {
         if ($data_value > $threshold['min'] && $data_value <= $threshold['max']) {
            $hex_color = $threshold['color'];
         }
      }
      
      return $this->hex_color_to_rgb_array($hex_color);
   
    }
    
    /**
     *  TODO: Remove this method.
     *  Make calls directly in the code or move to data manipulation object
     *
     */
    private function get_data($args) {
        
        $data_key = $args;
        if(is_array($args)) {
            $data_key = isset($args['data_key']) ? $args['data_key'] : 'team_name';
        }
        
        switch ($data_key) {
            case 'team_name':
                return preg_replace('/[^,;a-zA-Z0-9_\-\s]|[,;]$/s', '', $this->report_info['team_name']);
                //return $this->report_info['team_name'];
                break;
            case 'report_filename':
                $team_name = $this->report_info['team_name'] . '_' . $this->report_info['base_class'];
                $tmp = preg_replace('/^\W+|\W+$/', '', $team_name); // remove all non-alphanumeric chars at begin & end of string
                $tmp = preg_replace('/\s+/', '_', $tmp); // compress internal whitespace and replace with _
                $safe_team_name = strtolower(preg_replace('/\W-/', '', $tmp)); // remove all non-alphanumeric chars except _ and -
                
                return $safe_team_name . '.pdf';
                break;
            //case 'team_average':
            //    break;
            //case 'team_summary':
            //    break;
            //case 'member_mean_by_category':
            //    return $this->get_member_mean_by_category();
            //    break;
            //case 'team_mean_by_category':
            //    return $this->get_team_mean_by_category();
            //    break;
            case 'page_num':
                return $this->page_num; //t('Page @num', array('@num' => $this->page_num));
                break;
            
        }
    }
}