<?php
// $Id$

/**
 * @file
 * Creates Team Diagnostic reporting output.
 * 
 * The Team Diagnostic Report module builds a PDF file from custom Report Page
 * nodes in the Drupal CMS. Report Pages are tagged with a Diagnostic taxonomy
 * and ordered with node_references attached to the report Header Page.
 *
 * The dynamic elements in the Report Pages are implemented with embedded xml
 * objects. This module extracts the objects and processes them as HTML content,
 * injecting the HTML back into the Report Page's content.
 *
 * In this version, there are two types of XML objects to process:
 *   1. DataGrids
 *   2. Google Chart overlays.
 *
 * Available variables:
 * $line_id: order line item id. This uniquely identifies the team and
 *           diagnostic 
 *
 * DataGrid Syntax:
 * <triaxia type="datagrid" id="tableObjectID" style="background-color: #cccccc">
 *   <sql>SELECT response_id, finished FROM tptd_response WHERE order_line_item_id = :line_id</sql>
 *   <sqlparams>
 *    <param name="line_id">$line_id</param>
 *   </sqlparams>
 * </triaxia>
 *
 * Google Chart syntax:
 * <triaxia type="chart" id="chartObjectID" style="float:right">
 *   <chart type="scatterplot">
 *     <data>...</data>
 *   </chart>
 * </triaxia>
 * 
 */
require_once(libraries_get_path('tcpdf') . '/config/lang/eng.php');
require_once(libraries_get_path('tcpdf') . '/tcpdf.php');
define('LOGO_URI', 'public://logo.png');
define('LOGO_FILENAME', 'teambenchmark_logo.png');

// Extend the TCPDF class to create custom Header and Footer
class TeamDiagnosticPDF extends TCPDF {
  public $suppress_header = false;
  public $suppress_footer = false;
  
  //Page header
  public function Header() {
  //  if ($this->page == 1) {
  //    return;
  //  }
    if($this->suppress_header) {
      return;
    }
    // Logo
    //$logo = file_create_url(LOGO_URI);
    
    //parent::Header();
    
    if ($this->header_xobjid < 0) {
      // start a new XObject Template
      $this->header_xobjid = $this->startTemplate($this->w, $this->tMargin);
      $headerfont = $this->getHeaderFont();
      $headerdata = $this->getHeaderData();
      $this->y = $this->header_margin;
      if ($this->rtl) {
        $this->x = $this->w - $this->original_rMargin;
      } else {
        $this->x = $this->original_lMargin;
      }
      if (($headerdata['logo']) AND ($headerdata['logo'] != K_BLANK_IMAGE)) {
        $imgtype = $this->getImageFileType(K_PATH_IMAGES.$headerdata['logo']);
        if (($imgtype == 'eps') OR ($imgtype == 'ai')) {
          $this->ImageEps(K_PATH_IMAGES.$headerdata['logo'], '', '', $headerdata['logo_width']);
        } elseif ($imgtype == 'svg') {
          $this->ImageSVG(K_PATH_IMAGES.$headerdata['logo'], '', '', $headerdata['logo_width']);
        } else {
          $this->Image(K_PATH_IMAGES.$headerdata['logo'], '', '', $headerdata['logo_width']);
        }
        $imgy = $this->getImageRBY();
      } else {
        $imgy = $this->y;
      }
      $cell_height = round(($this->cell_height_ratio * $headerfont[2]) / $this->k, 2);
      // set starting margin for text data cell
      if ($this->getRTL()) {
        $header_x = $this->original_rMargin + ($headerdata['logo_width'] * 1.1);
      } else {
        $header_x = $this->original_lMargin + ($headerdata['logo_width'] * 1.1);
      }
      $cw = $this->w - $this->original_lMargin - $this->original_rMargin - ($headerdata['logo_width'] * 1.1);
      $this->SetTextColor(0, 0, 0);
      // header title
      $this->SetFont($headerfont[0], 'B', $headerfont[2] + 1);
      $this->SetX($header_x);
      $this->Cell($cw, $cell_height, $headerdata['title'], 0, 1, 'R', 0, '', 0);
      // header string
      $this->SetFont($headerfont[0], $headerfont[1], $headerfont[2]);
      $this->SetX($header_x);
      $this->MultiCell($cw, $cell_height, $headerdata['string'], 0, 'R', 0, 1, '', '', true, 0, false, true, 0, 'T', false);
      // print an ending header line
      $this->SetLineStyle(array('width' => 0.85 / $this->k, 'cap' => 'butt', 'join' => 'miter', 'dash' => 0, 'color' => array(0, 0, 0)));
      $this->SetY((2.835 / $this->k) + max($imgy, $this->y));
      if ($this->rtl) {
        $this->SetX($this->original_rMargin);
      } else {
        $this->SetX($this->original_lMargin);
      }
      $this->Cell(($this->w - $this->original_lMargin - $this->original_rMargin), 0, '', 'T', 0, 'C');
      $this->endTemplate();
    }
    // print header template
    $x = 0;
    $dx = 0;
    if ($this->booklet AND (($this->page % 2) == 0)) {
      // adjust margins for booklet mode
      $dx = ($this->original_lMargin - $this->original_rMargin);
    }
    if ($this->rtl) {
      $x = $this->w + $dx;
    } else {
      $x = 0 + $dx;
    }
    $this->printTemplate($this->header_xobjid, $x, 0, 0, 0, '', '', false);
    if ($this->header_xobj_autoreset) {
      // reset header xobject template at each page
      $this->header_xobjid = -1;
    }
    
  }
  // Page footer
  public function Footer() {

  //  if ($this->page == 1) {
  //    return;
  //  }
    
    if($this->suppress_footer || $this->page == 1) {  // $this->page == 1 is a hack to override some unknown TCPDF behavior.
      return;
    }
    else {
      $current_year = date('Y');
      $this->Cell(0, 10, "Copyright © 2000-$current_year Triaxia Partners, Inc. All Rights Reserved.", 0, false, 'C', 0, '', 0, false, 'T', 'M');
      parent::Footer();
    }
  }
}

function diagnostic_report() {
  $path_to_tcpdf = libraries_get_path('tcpdf');
  $args = func_get_args();
  $line_id = $args[0];
  
  $report_info = get_team_diagnostic_info_from_line_item($line_id);
  
  $report_id = $report_info['report_id']; // 15; // pass with args
  
  //TODO: refactor all code to pull distinct diagnostic report data.
  // using $line_id (or maybe pass $report_id), get report pages
  
  $report_node = node_load($report_id);
  $report_nids = $report_node->field_report_pages[$report_node->language];
  $nids = array();
  foreach($report_nids as $delta => $nid) {
    $nids[] = $nid['nid'];
  }
  $cover_url = file_create_url($report_node->field_cover_image[$report_node->language][0]['uri']);
  
  $report = array(
    'title' => $report_node->title,
    'report_nids' => $nids, //$report_nids,
    'line_item_id' => $line_id,
    'cover_url' => $cover_url,
    'team_name' => $report_info['team_name'],
  );
  
  createPDF($report);
  
  return render_report_html($report);
  
  //return '<h2>Sorry. There was an error in the attempt to generate this report. Please contact technical support.</h2>';
 
}

function render_report_html($report) {
   // Add pages
  $output = '<h2>' . $report['title'] . '&nbsp;Test</h2>';
  
  $report_nids = $report['report_nids'];
  foreach($report_nids as $delta => $nid) {
    $report_page = get_report_content($nid, $report['line_item_id']);
    $output .= $report_page['html'];
  }
  
  return $output;
}

function createPDF($report) {

  $title = $report['title'];
  //$logo = file_create_url(LOGO_URI);

  //$pdf = new TCPDF(PDF_PAGE_ORIENTATION, PDF_UNIT, PDF_PAGE_FORMAT, true, 'UTF-8', false);
  $pdf = new TeamDiagnosticPDF(PDF_PAGE_ORIENTATION, PDF_UNIT, "LETTER", true, 'UTF-8', false);
  
  // set document information
  $pdf->SetCreator(PDF_CREATOR);
  $pdf->SetAuthor('Triaxia');
  $pdf->SetTitle($title);
  $pdf->SetSubject('Team Benchmark Reporting');
  $pdf->SetKeywords('Team Benchmark, Diagnostics, Team');
  
  //set default header data
  $pdf->SetHeaderData(LOGO_FILENAME, 80, $title, $report['team_name']); // "Triaxia Team Diagnostics");
  // set header and footer fonts
  $pdf->setHeaderFont(Array(PDF_FONT_NAME_MAIN, '', PDF_FONT_SIZE_MAIN));
  $pdf->setFooterFont(Array(PDF_FONT_NAME_DATA, '', PDF_FONT_SIZE_DATA));
  
  // set default monospaced font
  $pdf->SetDefaultMonospacedFont(PDF_FONT_MONOSPACED);
  //set margins
  $pdf->SetMargins(15, 18, 15, true); //PDF_MARGIN_LEFT, PDF_MARGIN_TOP, PDF_MARGIN_RIGHT);
  $pdf->SetHeaderMargin(5);
  $pdf->SetFooterMargin(10);
  
  //set auto page breaks
  $pdf->SetAutoPageBreak(TRUE, PDF_MARGIN_BOTTOM);
  
  //set image scale factor
  $pdf->setImageScale(PDF_IMAGE_SCALE_RATIO);
  
  //set some language-dependent strings
  //$pdf->setLanguageArray($l);
  
  // set default font subsetting mode
  $pdf->setFontSubsetting(true);
  // Set font
  // dejavusans is a UTF-8 Unicode font, if you only need to
  // print standard ASCII chars, you can use core fonts like
  // helvetica or times to reduce file size.
  $pdf->SetFont('dejavusans', '', 12, '', true);
  
  //$pdf->setJPEGQuality(100);

  // ---------------------------------------------------------
  // add cover
  $pdf->suppress_footer = true;
  $pdf->suppress_header = true;
  
  $pdf->AddPage();

  // get the current page break margin
  $bMargin = $pdf->getBreakMargin();
  // get current auto-page-break mode
  $auto_page_break = $pdf->getAutoPageBreak();
  // disable auto-page-break
  $pdf->SetAutoPageBreak(false, 0);
  // set bacground image
  $cover_url = $report['cover_url'];
  global $base_url;
  $pdf->Image($cover_url, 0, 0, 216, 280, '', $base_url, 'T', false, 300, '', false, false, 1, false, false, false);
  // restore auto-page-break status
  $pdf->SetAutoPageBreak($auto_page_break, $bMargin);
  // set the starting point for the page content
  $pdf->setPageMark();  

  // CUSTOM PADDING
  // set color for background
  //$pdf->SetFillColor(255, 255, 215);
  // set font
  $pdf->SetFont('helvetica', '', 18);
  // set cell padding
  $pdf->setCellPaddings(1,1,1,1); //(2, 4, 6, 8);
  $txt = $report['team_name'];
  //$title_border = array('LTRB' => array('width' => .5, 'cap' => 'butt', 'join' => 'miter', 'dash' => 0, 'color' => array(125, 125, 125))) ;
  $pdf->MultiCell(183, 40, $txt, 0, 'R', 0, 1, 25, 52, true, 0, false, false);
  
  //$pdf->Output('team_diagnostic.pdf', 'I');
  //exit();
  
  // Add pages

  $report_nids = $report['report_nids'];
  $last_page = count($report_nids) - 1;
  $css_file = drupal_get_path('module', 'team_diagnostics') . '/report.css';
  $css = '';
  if(file_exists($css_file)) {
    $css_contents = file_get_contents($css_file);
    $css = '<style>' . $css_contents . '</style>';
  }
  
  foreach($report_nids as $delta => $nid) {
    // This method has several options, check the source code documentation for more information.
    
    $pdf->suppress_footer = $delta == $last_page ? true : false;
    $pdf->suppress_header = $delta == $last_page ? true : false;
    
    $pdf->AddPage();
    $pdf->SetFont('helvetica', '', 8);
    
    
    $report_content = get_report_content($nid, $report['line_item_id']);
    // set a page background image if the node contains a .page-background DIV
    foreach ($report_content['background-images'] as $bg_image) {
      $x_pos = (float)$bg_image['x']; //
      $y_pos = (float)$bg_image['y'];
      $width = ((float)$bg_image['width'] * 25.4) / 92.5;
      $height = ((float)$bg_image['height'] * 25.4) /92.5;
      
      $pdf->Image($bg_image['url'], $x_pos, $y_pos, $width, $height, '', '', '', true, 300, '', false, false, 1, false, false, false);
      $pdf->setXY(15, 18);
    }
    // Set HTML content to print
    $html = $css . $report_content['html'];
    // Print text using writeHTMLCell()
    //$pdf->writeHTMLCell($w=0, $h=0, $x='', $y='', $html, $border=0, $ln=1, $fill=0, $reseth=true, $align='', $autopadding=true);
    $pdf->writeHTMLCell(0, 0, '', '', $html);
    //$pdf->writeHTML($html, true, false, false, false, '');
    //$pdf->Image('http://www.google.com/intl/en_com/images/srpr/logo3w.png', 50, 70, 55, 19);
    
    $pdf->lastPage();
    //break; //TODO: Remove this line. It is for testing purposes only.
  }
 
  // ---------------------------------------------------------
    
  // Close and output PDF document
  // This method has several options, check the source code documentation for more information.
  $pdf->Output('team_diagnostic.pdf', 'I');
  exit();
}

function get_report_content($node, $line_item_id) {
  $html = $node;
  if (is_numeric($node)) {
    $page_node = node_load($node);
    $html = $page_node->body[$page_node->language][0]['value'];
  }
  
  $output = preg_replace('/[\n\r]/', ' ', $html); //$html; //
  $output = preg_replace('/>[\s]*?</', '><', $output);
  $elements; // = array();
  // get all the triaxia xml nodes in the content
  $count = preg_match_all('/<triaxia.*?<\/triaxia>/i', $output, $elements);
  
  while($count > 0) {
    $count--;
    $el = new SimpleXMLElement('<?xml version="1.0" encoding="UTF-8" standalone="no"?>' . $elements[0][$count]);
    $id = (string) $el['id']; //$el['id'];
    $type = (string) $el['type'];
    // get the pattern to replace this unique node
    $pattern = '/<triaxia\sid=[\'"]' . $id . '[\'"].*?>.*?<\\/triaxia>/i';
    
    $line_id = $line_item_id;
    if ($line_item_id == -1 && isset($el->data->default_line_item_id)) {
      $line_id = (int) $el->data->default_line_item_id;
    }
    
    // do some processing to create an HTML element from the xml node
    $new_element = '<div style="display:none"></div>';
    switch($type) {
      case 'datagrid':
        $new_element = render_datagrid($el, $line_id);
        break;
      case 'chart':
        $new_element = render_chart($el, $line_id);
        break;
      case 'repeater':
        $new_element = render_repeater($el, $line_id);
        break;
      case 'conditional':
        $new_element = render_conditional($el, $line_id);
        break;
    }
    // and then replace in back in the content string
    $output = preg_replace($pattern, $new_element, $output);
  }
  
  $bg_pattern = '/<div\sclass=[\'"]page-background[\'"].*?>.*?<\\/div>/i';
  $bg_images;
  $img_count = preg_match_all($bg_pattern, $html, $bg_images);
  $backgrounds = array();
  while($img_count > 0) {
    $img_count--;
    $backgrounds[] = parse_background_images($bg_images[0][$img_count]);
  }
  $report_content = array(
    'html' => $output,
    'background-images' => $backgrounds,
  );
  
  return $report_content; // $output;
}

function parse_background_images($bg_declaration = array()) {
  //TODO: Implement multiple background images
  
  $width_regex = 'width=[\'|"](\d+)';
  $width_match;
  $count = preg_match_all("/$width_regex/i", $bg_declaration, $width_match);
  $width = $count ? $width_match[1][0] : 500;
  
  $height_regex = 'height=[\'|"](\d+)';
  $height_match;
  $count = preg_match_all("/$height_regex/i", $bg_declaration, $height_match);
  $height = $count ? $height_match[1][0] : 500;
  
  $x_pos_regex = 'left:(\d+)';
  $x_pos_match;
  $count = preg_match_all("/$x_pos_regex/i", $bg_declaration, $x_pos_match);
  $x_pos = $count ? $x_pos_match[1][0] : 0;
  
  $y_pos_regex = 'top:(\d+)';
  $y_pos_match;
  $count = preg_match_all("/$y_pos_regex/i", $bg_declaration, $y_pos_match);
  $y_pos = $count ? $y_pos_match[1][0] : 0;
  
  $url_regex = 'src=[\'|"](.+?)[\'|"]';
  $url_match;
  $count = preg_match_all("/$url_regex/i", $bg_declaration, $url_match);
  $url = $count ? $url_match[1][0] : '';
  
  
  return array(
          'url' => $url,
          'x' => $x_pos,
          'y' => $y_pos,
          'width' => $width,
          'height' => $height,
        );
}

function render_datagrid($element, $line_item_id) {
  //$line_id = 1;
  $sql = (string) $element->data->sql;
  $param_array = array(':line_item_id' => $line_item_id);
  $header_style = (string) $element->settings->header_style;
  $row_style = (string) $element->settings->row_style;

  $headers = array();
  $header_str = (string) $element->data->headers;
  $header_cells = explode(',', $header_str); //preg_split('/,/', );
  foreach($header_cells as $cell) {
    $headers[] = array('data' => $cell, 'style' => $header_style);
  }
  
  $column_formats = array();
  if(isset($element->data->column_formatting)) {
    foreach ($element->data->column_formatting->column as $column) {
      $col_name = (string) $column['name'];
      $col_type = (string) $column['type'];
      $column_formats[$col_name]['type'] = $col_type;
      if(isset($column['precision'])) {
        $col_precision = (int) $column['precision'];
        $column_formats[$col_name]['precision'] = $col_precision;
      }
      if (isset($column['appearance'])) {
        $col_appearance = (string) $column['appearance'];
        $column_formats[$col_name]['appearance'] = $col_appearance;
      }
    }
  }
  
  $rows = array();
  $data_rows = get_array_from_sql($sql, $param_array);
  foreach($data_rows as $data_row) {
    $row = array();
    foreach($data_row as $col => $datum) {
      $data_value = format_data_cell($col, $datum, $column_formats);
      $style = $row_style;
      if (isset($column_formats[$col]['appearance'])) {
        $style = $column_formats[$col]['appearance']; 
      }
      $row[] = array('data' => $data_value, 'style' => $style);
    }
    $rows[] = $row;
  }
  
  if(isset($element->data->footers)) {
    //$inspect_this = $element;
    $sql = $element->data->footers->sql;
    $footer_data = get_array_from_sql($sql, $param_array);
    $footer_style = (string) $element->settings->footer_style;
    //$footer_rows = array();
    foreach($footer_data as $data_row) {
      $row = array();
      foreach($data_row as $col => $data_cell) {
        $data_value = format_data_cell($col, $data_cell, $column_formats);
        $row[] = array('data' => $data_value, 'style' => $footer_style);
      }
      $rows[] = $row;
      
      if (isset($element->data->footers['has_ranking'])) {
        $has_ranking = (boolean) $element->data->footers['has_ranking'];
        if ($has_ranking) {
          $ranking_range_str = (string) $element->data->footers['ranking_col_range'];
          $ranking_range = explode(',', $ranking_range_str);
          
          $rank_style = array('column_label' => 'Rank', 'styles' => array('row_style' => $footer_style));
          $rank_style['styles']['highest'] = 'background-color:green';
          $rank_style['styles']['lowest'] = 'background-color:red';
          if (isset($element->settings->ranking_styles)) {
            $highest = (string) $element->settings->ranking_styles->highest;
            $lowest = (string) $element->settings->ranking_styles->lowest;
            $rank_style['styles']['highest'] = $highest;
            $rank_style['styles']['lowest'] = $lowest;
          }
          
          $ranked_row = build_ranking_row($row, $ranking_range, $rank_style);
          $rows[] = $ranked_row;
        }
      }
    }
  }
  
  $tablevars = array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array('id' => $element['id']), // 'purpose-table'),
    'caption' => NULL,
    'colgroups' => NULL,
    'sticky' => 0,
    'empty' => NULL,
  );
  
  return theme_table($tablevars);
}

function format_data_cell($column, $datum, $formats) {

  if(!array_key_exists($column, $formats)) {
    return $datum;
  }
  
  $format = $formats[$column];
  $return_value = $datum;
  switch ($format['type']) {
    case 'int':
    case 'integer':
      $return_value = (float) $datum;
      break;
    case 'float':
      $return_value = (float) $datum;
      if (isset($format['precision'])) {
        $return_value = round($return_value, $format['precision']);
        $return_value = number_format($return_value, 2, '.', '');
      }
      break;
    case 'currency':
    case 'money':
      break;
  }
  
  return $return_value;
}

function build_ranking_row($row, $range, $settings = array('column_label' => 'Rank', 'styles' => array('row_style' => 'background-color:#cccccc'))) {
  $ranked_row = array();
  $ranked_row[] = array('data' => $settings['column_label'], 'style' => $settings['styles']['row_style']);
  
  $cells_to_evaluate = array();
  for ($i = $range[0]; $i <= $range[1]; $i++ ) {
    $cells_to_evaluate[$i] = (float) $row[$i]['data'];
  }
  
  // sort $cells_to_evaluate by data and loop through giving ranking number
  $sorted_cells = array();
  asort($cells_to_evaluate);
  $rank = $range[1];
  foreach ($cells_to_evaluate as $delta => $cell) {
    $sorted_cells[$delta] = array('data' => $cell, 'rank' => $rank--);
  }
  
  // add these to resulting rank array in original order
  for ($i = $range[0]; $i <= $range[1]; $i++ ) {
    $style = $settings['styles']['row_style'];
    if ($sorted_cells[$i]['rank'] == 1) {
      $style = $settings['styles']['highest'];
    }
    if ($sorted_cells[$i]['rank'] == count($sorted_cells)) {
      $style = $settings['styles']['lowest'];
    }
    $ranked_row[] = array('data' => $sorted_cells[$i]['rank'], 'style' => $style);
  }
  
  // append empty cells as necessary
  $difference = count($row) - count($ranked_row);
  while ($difference-- > 0) {
    $ranked_row[] = array('data' => '&nbsp;', 'style' => $settings['styles']['row_style']);
  }
  
  return $ranked_row;
}

function render_chart_api_hybrid($element, $line_item_id) {
    
  $sql = (string) $element->data->sql;
  $param_array = array(':line_item_id' => $line_item_id);
  
   // set data points
   $chart = array();
  foreach($data_array as $delta => $row) {
    foreach($data_columns as $column) {
      $chart['#data'][$column][] = $row[$column];
    }
  }
  $chart_data_args = _enc_chart_encode_data($chart['#data']);
  
  $title = (string) $element->settings->title;
  $position = (string) $element->settings->position;
  $size_str = (string) $element->settings->size;
  $size = explode(',', $size_str);
  $chart_url = 'http://chart.apis.google.com/chart?' . (string) $element->settings->chart_args . '&position=' . $position;
  
  $variables = array(
    'path' => $chart_url,
    'alt' => $title,
    'title' => $title,
    'attributes' => array('width' => $size[0], 'height' => $size[1]),
  );
  
  return theme_image($variables);
  
}

function get_pchart_type_from_google_type($google_type) {
  $pchart_type = array('type' => 'bar', 'orientation' => 'horizontal');
  switch($google_type) {
    case 'bhs':
      $pchart_type['type'] = 'bar';
      $pchart_type['orientation'] = 'horizontal';
      break;
    case 'bvs':
      $pchart_type['type'] = 'bar';
      $pchart_type['orientation'] = 'vertical';
      break;
    case 's':
      $pchart_type['type'] = 'scatterplot';
      break;
    case 'r':
    case 'rs':
      $pchart_type['type'] = 'radar';
      break;
  }
  
  return $pchart_type;
}

function get_rgb_array_from_hex($hex_color_array) {
  $rgb = array();
  foreach ($hex_color_array as $delta => $color) {
    if (strlen($color) == 6)
        list($r, $g, $b) = array($color[0].$color[1],
                                 $color[2].$color[3],
                                 $color[4].$color[5]);
    elseif (strlen($color) == 3)
        list($r, $g, $b) = array($color[0].$color[0], $color[1].$color[1], $color[2].$color[2]);
    else
        return false;

    $r = hexdec($r); $g = hexdec($g); $b = hexdec($b);
    
    $rgb[$delta] = array(
      "R" => $r,
      "G" => $g,
      "B" => $b,
      "Alpha" => 100,
    );
  }
  return $rgb;
}

function new_render_chart($element, $line_item_id, $data_array) {
  
  $chart_id = (string) $element['id'];
  $type = (string) $element->settings->type;
  $chart_type = get_pchart_type_from_google_type($type);
  $palette = isset($element->settings->palette) ? (string) $element->settings->palette : 'summer.color';
  $size_str = (string) $element->settings->size;
  $size = explode(',', $size_str);
  $data_colors = explode('|', (string) $element->settings->data_colors);
  $color_override = get_rgb_array_from_hex($data_colors);
  
  $graph_margins = array(1,1,1,100);
  if (isset($element->settings->graph_margins)) {
    $graph_str = (string) $element->settings->graph_margins;
    $graph_margins = explode(',', $graph_str);
  }
  
  $x_axis_type = (string) $element->data->x_axis['type'];
  $x_axis = (string) $element->data->x_axis;
  
  $x_range = explode(',', $x_axis);
  $axis_boundaries = array(
    'Min' => $x_range[0],
    'Max' => $x_range[count($x_range) - 1],
  );
  
  //$scale_range = (string) $element->data->scale_range;
  
  $y_axis_type = (string) $element->data->y_axis['type'];
  $y_axis = (string) $element->data->y_axis; // this is the abscissa label column!
                                             // It's y-axis only in horizontal bar charts.
  
  
  $format = isset($element->data->format) ? (string) $element->data->format : 'int';
  $format_precision = (integer) $element->data->format['precision'];
  
  if (isset($element->data->sql)) {
    $sql = (string) $element->data->sql;
    $param_array = array(':line_item_id' => $line_item_id);
    $data_array = get_array_from_sql($sql, $param_array);
  }
  
  // re-order $data_array if node has provided abscissa labels
  if (isset($element->data->abscissa_labels)) {
    $abscissa_labels = explode(',', (string) $element->data->abscissa_labels);
    $re_ordered_data = array();
    foreach ($abscissa_labels as $index => $label) {
      foreach ($data_array as $delta => $row) {
        if ($row[$y_axis] == $label) {
          $re_ordered_data[$index] = $row;
        }
      }
    }
    $data_array = $re_ordered_data;
  }
  
  $data_columns_str = (string) $element->data->data_columns;
  //$data_columns_str = strtolower($data_columns_str);
  $data_columns = explode(',', $data_columns_str);
  
  // set data points
  $abscissa = array(
    'name' => isset($element->data->abscissa_labels['name']) ? (string) $element->data->abscissa_labels['name'] : 'Labels', //$y_axis,
    'values' => array(),
  );
  $series = array();
  foreach($data_array as $delta => $row) {
    
    foreach($data_columns as $column) {
      $data_point = $format == 'float' ? (float)$row[$column] : (int)$row[$column];
      if (isset($element->data->transformation)) {
        $op = (string) $element->data->transformation['op'];
        $val = (float) $element->data->transformation['value'];
        switch ($op) {
          case '*':
            $data_point = (float) $row[$column] * $val;
            break;
          case '^':
            $data_point = (float) pow((float) $row[$column],$val);
          default:
             break;
        }
      }
      
      if (is_float($data_point)) {
        $data_point = round((float)$data_point, $format_precision);
        $data_point = number_format($data_point, $format_precision, '.', '');
      }
      $series[$column][] = $data_point;
      if ($y_axis){
        $abscissa['values'][] = $row[$y_axis];
      }
    }
  }
  
  $actual_width = (int) $size[0];
  $actual_height = (int) $size[1];
  $magnification = 2;
  
  $data_image = isset($element->data->icon) ? (string) $element->data->icon : 'scatterpoint1-lg.png';
  $pchart_info = array(
    'id' => $chart_id . '_' . $line_item_id,
    'data' => array(
      'abscissa' => $abscissa,
      'series' => $series,
      'icon' => array(0 => PCHART_RESOURCE_DIRECTORY . $data_image),
    ),
    
    'chart' => array(
      'type' => $chart_type,
      'graph' => array(
        'top-margin' => (int) $graph_margins[0] * $magnification,
        'right-margin' => (int) $graph_margins[1] * $magnification,
        'bottom-margin' => (int) $graph_margins[2] * $magnification,
        'left-margin' => (int) $graph_margins[3] * $magnification,
      ),
    ),
    
    'scale' => array(
      'axis_boundaries' => $axis_boundaries,
      'x-margin' => 40,
      'y-margin' => 30,
    ),
    
    'settings' => array(
      'size' => array('width' => (int) $actual_width * $magnification, 'height' => (int) $actual_height * $magnification),
      //'size' => array('width' => (int) $actual_width, 'height' => (int) $actual_height),
      'font-name' => 'MankSans-Medium',
      'font-size' => isset($element->settings->font_size) ? (string) $element->settings->font_size : 22, //$chart_type['type'] == 'scatterplot' ? '6' : '22',
      'palette' => $palette,
      'color_override' => $color_override,
    ),
  );
  
  $chart = DrupalpChartFactory::create_chart($pchart_info);
  $chart_url = $chart->render();
  
  
  $variables = array(
    'path' => $chart_url,
    'alt' => 'Test',
    'title' => 'Test',
    'attributes' => array('width' => $actual_width, 'height' => $actual_height),
  );
  
  return theme_image($variables);
  
}

function render_chart($element, $line_item_id, $data_array = array()) {
  
  return new_render_chart($element, $line_item_id, $data_array);
  return;

  $chart_id = (string) $element['id'];
  $title = (string) $element->settings->title;
  $type = (string) $element->settings->type;
  $size_str = (string) $element->settings->size;
  $size = explode(',', $size_str);
  $data_colors = explode(',', (string) $element->settings->data_colors);
  $chart_fill = explode(',', (string) $element->settings->chart_fill);
  
  $chart = array(
    '#chart_id' => $chart_id,
    '#title' => '', //chart_title($title, '3333ff', 22),
    '#type' => $type,
    '#size' => chart_size($size[0], $size[1]),
    '#data_colors' => $data_colors,
    '#chart_fill' => $chart_fill,
  );
  
  $x_axis_type = (string) $element->data->x_axis['type'];
  $x_axis = (string) $element->data->x_axis;
  $y_axis_type = (string) $element->data->y_axis['type'];
  $y_axis = (string) $element->data->y_axis;
  $scale_range = (string) $element->data->scale_range;
  
  if (isset($element->data->sql)) {
    $sql = (string) $element->data->sql;
    $param_array = array(':line_item_id' => $line_item_id);
    $data_array = get_array_from_sql($sql, $param_array);
  }
  
  $data_columns_str = (string) $element->data->data_columns;
  $data_columns = explode(',', $data_columns_str);
  
  // set data points
  foreach($data_array as $delta => $row) {
    foreach($data_columns as $column) {
      $data_point = $row[$column];
      if (isset($element->data->transformation)) {
        $op = (string) $element->data->transformation['op'];
        $val = (float) $element->data->transformation['value'];
        switch ($op) {
          case '*':
            $data_point = (float) $row[$column] * $val;
            break;
          case '^':
            $data_point = pow((float) $row[$column],$val);
          default:
             break;
        }
      }
      
      $chart['#data'][$column][] = $data_point; // $row[$column];
    }
  }
  
  // set Y-AXIS
 switch ($y_axis_type) {
  case 'data_column':
    $row_num = count($data_array);
    while ($row_num-- > 0) {
      $chart['#mixed_axis_labels'][CHART_AXIS_Y_LEFT][0][] = chart_mixed_axis_label($data_array[$row_num][$y_axis]);
    }
    break;
  case 'custom_labels':
    $y_axis_labels = explode(',', $y_axis);
    foreach($y_axis_labels as $label) {
      $chart['#mixed_axis_labels'][CHART_AXIS_Y_LEFT][0][] = chart_mixed_axis_label($label);
    }
    break;
  case 'scale_range':
    $chart['#mixed_axis_labels'][CHART_AXIS_Y_LEFT][0][] = chart_mixed_axis_range_label($y_axis);
    break;
 }
 
  // set X-AXIS
 switch ($x_axis_type) {
  case 'data_column':
    foreach($data_array as $delta => $row) {
      $chart['#mixed_axis_labels'][CHART_AXIS_X_BOTTOM][0][] = chart_mixed_axis_label($data_array[$delta][$x_axis]);
    }
    break;
  case 'custom_labels':
    $x_axis_labels = explode(',', $x_axis);
    foreach($x_axis_labels as $label) {
      $chart['#mixed_axis_labels'][CHART_AXIS_X_BOTTOM][0][] = chart_mixed_axis_label($label);
    }
    break;
  case 'scale_range':
    //$chart['#mixed_axis_labels'][CHART_AXIS_X_BOTTOM][0][] = chart_mixed_axis_range_label($x_axis);
    $chart['#scale_range'] = explode(',', $x_axis);
    break;
 }
  
  if(!isset($chart['#scale_range'])) {
    $chart['#scale_range'] = explode(',', $scale_range);
  }
  
  $position = isset($element->settings) ? '&position=' . (string) $element->settings->position : '';
  $additional_chart_args = isset($element->settings->chart_args ) ?  (string) $element->settings->chart_args : '';
  $shape_marker = isset($element->settings->shape_marker) ? '&chm=' . (string) $element->settings->shape_marker : '';
  
  $chart_url = chart_url($chart) . $additional_chart_args . $shape_marker . $position;
  
  
  if (isset($element->settings->override_api)) {
    foreach ($element->settings->override_api->children() as $attr_name => $setting) {
      $attr = (string) $attr_name; //$setting;
      $value = (string) $setting;
      
      $keyval = $attr . '=' . $value;
      $look_for = '/[\?&]'. $attr . '=.*?[&$]/i';
      
      // 1. Look for the attribute in the current chart url
      $match_count = preg_match_all($look_for, $chart_url, $matches);
      
      // 2. if there's a match, replace it
      if ($match_count > 0) {
        $first_char = substr($matches[0][0], 0, 1);
        $last_char = substr($matches[0][0], -1, 1);
        $keyval_url   = $first_char . $keyval . $last_char;
        $chart_url = preg_replace($look_for, $keyval_url, $chart_url);
      }
      else {
        $chart_url .= '&' . $keyval;
      }
    }
  }
  
  $variables = array(
    'path' => $chart_url,
    'alt' => $title,
    'title' => $title,
    'attributes' => array('width' => $size[0], 'height' => $size[1]),
  );
  
  return theme_image($variables);
}

function encode_google_chart_url($chart_url) {
  
  $chart_base = 'http://chart.apis.google.com/chart?';
  $base_len = strlen($chart_base);
  $raw_query_str = substr($chart_url, $base_len);
  $encoded_query = str_replace('%3D', '=', rawurlencode($raw_query_str));
  $encoded_query = str_replace('%26', '&', $encoded_query);
  
  return $chart_base . $encoded_query;
}
function get_google_chart_data_stream($chart_url) {
  
  $chart_base = 'http://chart.apis.google.com/chart?';
  $base_len = strlen($chart_base);
  $raw_query_str = substr($chart_url, $base_len);
  $encoded_query = str_replace('%3D', '=', rawurlencode($raw_query_str));
  $encoded_query = str_replace('%26', '&', $encoded_query);
  
  $chart_url = $chart_base . $encoded_query;
  if ($stream = fopen($chart_url, 'r')) {
    $url_image= stream_get_contents($stream, -1);
    fclose($stream);
  }
    
  return '@' . $url_image;
}

function render_repeater($element, $line_item_id, $data_array = array()) {
  
  $grouping_field = (string) $element->group['change_on'];
  $group_prefix = (string) $element->group->prefix;
  $group_suffix = (string) $element->group->suffix;

  $item_definitions = array();
  $index = 0;
  foreach($element->group->items->item as $item_def) {
    $item_definitions[$index]['field'] = (string) $item_def['field'];
    $item_definitions[$index]['type'] = (string) $item_def['type'];
    $item_definitions[$index]['action'] = (string) $item_def['action'];
    if (isset($item_def['title'])) {
      $item_definitions[$index]['#title'] = $item_def['title'];
    }
    if ($item_def['type'] == 'markup') {
      $item_definitions[$index]['#markup'] = (string) $item_def;
    }
    if ($item_def['type'] == 'chart') {
      $item_definitions[$index]['#chart'] = $item_def->chart;
    }
    $index++;
  }
  
  if (isset($element->data->sql)) {
    $sql = (string) $element->data->sql;
    $param_array = array(':line_item_id' => $line_item_id);
    $data_array = get_array_from_sql($sql, $param_array);
  }
  
  $items = array();
  $index = 0;
  $last_group = 'c20ad4d76fe97759aa27a0c99bff6710'; // init w/ dummy group name
  foreach($data_array as $delta => $data_row) {
   
    // loop through the entire dataset and group data as declared for each item
    // add a new item group on change of indicated field
    if ($data_row[$grouping_field] != $last_group) {
      $index++;
      $items[$index] = array();
      foreach ($item_definitions as $item_def) {
        $items[$index][$item_def['field']] = array(
          '#type' => $item_def['type'],
          'action' => $item_def['action'],
        );
        if (isset($item_def['#title'])) {
          $items[$index][$item_def['field']]['#title'] = $item_def['#title'];
        }
        if ($item_def['type'] == 'markup') {
          $items[$index][$item_def['field']]['#markup'] = $item_def['#markup'];
        }
        if ($item_def['type'] == 'chart') {
          $items[$index][$item_def['field']]['#chart'] = $item_def['#chart'];
        }
      }
      $last_group = $data_row[$grouping_field];
    }
    
    // loop through item definitions and perform appropriate actions
    foreach ($item_definitions as $item_def) {
      if (strlen($data_row[$item_def['field']]) < 1) {
        continue;
      }
      switch ($item_def['action']) {
        case 'group':
          $data = $data_row[$item_def['field']];
          if ($item_def['field'] == 'item_text') {
            $data = $data_row['q_num'] . '. ' . $data;
          }
          $items[$index][$item_def['field']]['data'] = $data; //$data_row[$item_def['field']];
          break;
        case 'avg':
        case 'sum':
        case 'mean':
        case 'list':
        case 'detail':
        case 'max':
        case 'min':
        case 'count':
          $items[$index][$item_def['field']]['data'][] = $data_row[$item_def['field']];
          break;
      }
      //$items[$index][$item_def['field']]
    }
  }
  
  // transform items for drupal_render and perform grouping actions
  foreach ($items as $delta => $group) {
    foreach ($group as $field => $item) {
      
      switch ($item['#type']) {
        case 'markup':
          $data;
          if($item['action'] == 'group') {
            $data = $item['data'];
          }
          else {
            $data = calculate_group($item['data'], $item['action']);
          }
          //$markup = t($item['#markup'], array('%0' => $data)); // fill variable into markup
          $markup = str_replace('%0', $data, $item['#markup']); // fill variable into markup
          $items[$delta][$field]['#markup'] = '<p>&nbsp;</p>' . $markup;
          break;
        case 'chart':
          $data = calculate_group($item['data'], $item['action']);
          
          $chart = $item['#chart'];
          $chart['id'] = $chart['id'] . '_' . $delta;
          $y_axis = (string) $chart->data->y_axis;
          $data_columns_str = (string) $chart->data->data_columns;
          $data_columns = explode(',', $data_columns_str);
          $col_num = count($data_columns);
          $chart_data = array();
          for ($i = 0; $i < $col_num; $i++) {
            $chart_data[$i][$data_columns[$i]] = $data; //TODO: Refactor for multiple data values
            $chart_data[$i][$y_axis] = $y_axis;
          }
          
          $output = render_chart($chart, $line_item_id, $chart_data);
          
          $items[$delta][$field]['#type'] = 'markup';
          $items[$delta][$field]['#markup'] = $output; // for testing
          $items[$delta][$field]['#suffix'] = '<br><br>'; // for testing
          break;
        case 'ul':
        case 'ol':
          if (!isset($item['data'])) {
            break;
          }
          $list = $item['data'];
          if (isset($item['#title'])) {
            $items[$delta][$field]['#title'] = (string) $item['#title'];
          }
          $items[$delta][$field]['#prefix'] = '<p>&nbsp;</p>';
          //$items[$delta][$field]['#suffix'] = '</p>';
          $items[$delta][$field]['#theme'] = 'item_list';
          $items[$delta][$field]['#items'] = $list;
          break;
      }
      
      unset($items[$delta][$field]['data']);
      unset($items[$delta][$field]['action']);
      
    }
  }
  
  return drupal_render($items);
}

function calculate_group($data, $action) {
  
  $group_result;
  switch ($action) {
    case 'group':
      if (is_array($data)) {
        $data = $data[0];
      }
      $group_result = $data;
      break;
    case 'mean':
    case 'avg':
      $sum = 0;
      $total = 0;
      foreach ($data as $val) {
        if($val > 0) {
          $total++;
          $sum += $val;
        }
      }
      if ($total > 0) {
        $group_result = $sum / $total;
      }
      break;
    case 'sum':
      $group_result = array_sum($data);
      break;
    case 'count':
      $group_result = count($data);
      break;
    case 'max':
      $group_result = max($data);
      break;
    case 'min':
      $group_result = min($data);
      break;
    case 'list':
    case 'detail':
      $group_result = $data;
      break;
  }
  
  return $group_result;
}

function render_conditional($element, $line_item_id, $data_array = array()) {
  
  $data_val = 3;
  
  if (isset($element->data->sql)) {
    $sql = (string) $element->data->sql;
    $param_array = array(':line_item_id' => $line_item_id);
    $data_val = db_query($sql, $param_array)->fetchField(); // get_array_from_sql($sql, $param_array);
  }
  
  $items = array();
  
  foreach($element->items->item as $item) {
    $format = (string) $item['default'];
    $condition = (string) $item->condition;
    $condition = str_replace("%d", $data_val, $condition); //, array('%d' => $data_val));
    //$condition = str_replace("%s", $data_val, "'" . $condition . "'"); //, array('%d' => $data_val));
    $match = false;
    $eval_str = '$match = ' . $condition . ';';
    eval($eval_str);
    
    if($match) {
      $format = (string) $item['format'];
    }
    
    $tag = 'div';
    if (isset($item['tag'])) {
      $tag = (string) $item['tag'];
    }
    
    if ($format != 'display:none') {
      $items[] = array(
        'type' => 'markup',
        '#markup' => "<$tag style=\"$format\">$item->text</$tag>",
      );
    }
  }
  
  $inspect = $items;
  return drupal_render($items);
}