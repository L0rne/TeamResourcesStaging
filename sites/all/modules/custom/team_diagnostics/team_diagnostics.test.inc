<?php
// $Id$

/**
 * @file
 * Creates Team Diagnostic reporting output.
 *
 */

require_once(dirname(__FILE__) . '/team_diagnostics.data-access.inc');
require_once(dirname(__FILE__) . '/team_diagnostics.pdflib.inc');
require_once(dirname(__FILE__) . '/team_diagnostics.css-parser.inc');
require_once(dirname(__FILE__) . '/team_diagnostics.data-manipulation.inc');
require_once(dirname(__FILE__) . '/team_diagnostics.pdflib-chart.inc');

require_once(libraries_get_path('pchart') . '/class/pData.class.php');
require_once(libraries_get_path('pchart') . '/class/pDraw.class.php');
require_once(libraries_get_path('pchart') . '/class/pImage.class.php');
require_once(libraries_get_path('pchart') . '/class/pPie.class.php');
require_once(libraries_get_path('pchart') . '/class/pRadar.class.php');
require_once(libraries_get_path('pchart') . '/class/pScatter.class.php');



function report_xml_tester() {
  
  test_team_diagnostic_member_numbers(148, 'Testing the Member Numbers');
  return '<h3>I\'m not quite dead yet.</h3>';
}

/**
 * Test for mapping the DMO member display numbers to their response data
 * 
 * test_team_diagnostic_member_numbers(148, 'Testing the Member Numbers');
 */
function test_team_diagnostic_member_numbers($line_item_id, $team_name) {
  $dmo = new DiagnosticDMO($line_item_id, $team_name);
  $dmo_dataset = $dmo->get_data();
  $db_dataset = test_team_diagnostic_refactored_data_fetch($line_item_id);
  
  echo '<table><tr><th>Response ID</th><th>Member ID</th></tr>';
  foreach ($db_dataset['items'] as $delta => $item) {
    echo '<tr><td>';
    
    foreach($item['responses'] as $r_id => $r_data) {
      echo $r_id . ' : ' . $r_data . '<br />';
    }
    
    echo '</td><td>';
    
    foreach ($dmo_dataset['items'][$delta]['responses'] as $m_id => $m_data) {
      echo $m_id . ' : ' . $m_data . '<br />';
    }
    
    echo '</td></tr>';
  }
  echo '</table>';
  
  exit();
}

function test_team_diagnostic_refactored_data_fetch($line_item_id) {
    // joins
    $query = db_select('tptd_response_details', 'd');
    $query->innerJoin('tptd_survey_items', 'i', 'd.item_id = i.item_id');
    $query->innerJoin('tptd_response', 'r', 'd.response_id = r.response_id');
    // fields
    $query->addField('r', 'response_id');
    $query->addField('r', 'team_member_email');
    $query->addField('i', 'item_id');
    $query->addField('i', 'item_type');
    $query->addField('i', 'item_text');
    $query->addField('i', 'category');
    $query->addField('i', 'delta');
    $query->addField('d', 'points');
    $query->addField('d', 'comments');
    // conditions
    $query->condition('r.order_line_item_id', $line_item_id, '=');
    //$query->condition('d.points', 0, '>');
    $query->orderBy('i.delta', 'ASC');
    $query->orderBy('r.response_id', 'ASC');
    
    $result = $query->execute();
    
    $categories = array();
    $items = array();
    $responses = array();
    $participants = array();
    $comments = array();
    while ($record = $result->fetchAssoc()) {
          // structure by items
          if (!in_array($record['category'], $categories)) {
            $categories[] = $record['category'];
          } //TODO: implement "report order" for categories
          
          $participants[$record['response_id']] = $record['team_member_email'];
          
          if (strlen($record['comments']) > 1) {
            $comments[$record['item_id']][$record['response_id']] = $record['comments'];
          }
          
          if ((float)$record['points'] == 0) continue; // don't include zero-answers. They are non-answers in this context.
          
          $responses[$record['delta']][$record['response_id']] = (float)$record['points'];
          $items[$record['delta']] = array(
            'id'        => (int)$record['item_id'],
            'category'  => $record['category'],
            'q_num'     => 'Q' . ($record['delta'] + 1),
            'text'      => $record['item_text'],
            'type'      => $record['item_type'],
            'responses' => $responses[$record['delta']],
          );
        }
    
    return array(
        'team' => $participants,
        'categories' => $categories,
        'items' => $items,
        'comments' => $comments,
    );
    
}

/**
 * Test Diagnostic Report PDF Generation
 * //test_diagnostic_pdf_report(201, 'dynamic-y-meetings.pdf');
*/
function test_diagnostic_pdf_report($line_id, $template_filename) {
  //$line_id = 198; // 198 (comm); 199 (conflict); 148 (survey); 2 (profile);  201 (meetings)
  
  $report_info = get_team_diagnostic_info_from_line_item($line_id);
  
  $report_id = $report_info['report_id'];  
  $report_node = node_load($report_id);
  
  $report_info['line_item_id'] = $line_id;
  $report_info['base_class'] = $report_node->field_base_class[$report_node->language][0]['safe_value'];
  $report_info['default_category_order'] = $report_node->field_default_category_order[$report_node->language][0]['safe_value'];
  $report_info['data_color_thresholds'] = $report_node->field_data_color_range[$report_node->language];
  $report_css_file = drupal_get_path('module', 'team_diagnostics') . '/pdf.css';
  $style = new triaxia_CSSParser();
  $style->Parse($report_css_file);
  $report_info['css'] = $style;
  
  //$template_filename = $report_node->field_pdf_template[$report_node->language][0]['filename'];
  //$template_filename = 'team-development-profile-v.1.1.pdf';
  //$template_filename = 'team-meetings-v.1.1.pdf';
  // $template_filename ='team-conflict-template.v.1.1.pdf';
  //$template_filename = 'survey-blocks.pdf';
  //$template_filename ='team-communication-template.v.1.1.pdf';
  $template_filename = 'Team-Performance-Survey v17.pdf';
  
  $pdf = new DiagnosticPDF($report_info);
  $pdf->merge_template($template_filename);
  $pdf->print_pdf();
  exit();
}

function PDFLib_test() {
  
  try {
      $p = new PDFlib();
      
      # This means we must check return values of load_font() etc.
      $p->set_parameter("errorpolicy", "return");
  
      /* This line is required to avoid problems on Japanese systems */
      $p->set_parameter("hypertextencoding", "winansi");
  
      /*  open new PDF file; insert a file name to create the PDF on disk */
      if ($p->begin_document("", "") == 0) {
          die("Error: " . $p->get_errmsg());
      }
  
      $p->set_info("Creator", "hello.php");
      $p->set_info("Author", "Rainer Schaaf");
      $p->set_info("Title", "Hello world (PHP)!");
  
      $p->begin_page_ext(595, 842, "");
  
      $font = $p->load_font("Helvetica-Bold", "winansi", "");
      if ($font == 0) {
          die("Error: " . $p->get_errmsg());
      }
  
      $p->setfont($font, 24.0);
      $p->set_text_pos(50, 700);
      $p->show("Hello world!");
      $p->continue_text("(says PHP)");
      $p->end_page_ext("");
  
      $p->end_document("");
      
      $buf = $p->get_buffer();
      $len = strlen($buf);
  
      header("Content-type: application/pdf");
      header("Content-Length: $len");
      header("Content-Disposition: inline; filename=hello.pdf");
      print $buf;
      
      exit();
  
  }
  catch (PDFlibException $e) {
      die("PDFlib exception occurred in hello sample:\n" .
          "[" . $e->get_errnum() . "] " . $e->get_apiname() . ": " .
          $e->get_errmsg() . "\n");
  }
  catch (Exception $e) {
      die($e);
  }
  
  $p = 0;
  
  return '<h3>Did not work</h3>';
  

}

function txp_test_prep_for_team_page() {
  $report_css_file = dirname(__FILE__) . '/pdf.css';
  $style = new triaxia_CSSParser();
  $style->Parse($report_css_file);
  
  $report_info = get_team_diagnostic_info_from_line_item(1);
  $report_info['line_item_id'] = 1;
  $report_info['base_class'] = 'survey';
  $report_info['css'] = $style;
  
  $pdf = new DiagnosticPDF($report_info);
  $pdf->merge_template('team-skill-survey-master-v6.pdf');
  //$pdf->merge_template('team-development-profile.pdf');
  //$pdf->merge_template('survey-blocks.pdf');
  //$pdf->merge_template('block-tester.pdf');
  //$pdf->merge_template('nested-block-test-template.pdf');
  //$pdf->merge_template('team-development-profile-master-2.pdf');
  
  
  $pdf->print_pdf();
  exit();
  
  return '<h3>Test Complete!</h3>';
}
function txt_test_replacement_string() {
  $data_args = array('data_key' => 'min_team_mean_category');
  $dmo = new DiagnosticDMO(1);
  $str = 'Overall, !weakest has rated the weakest characteristic for your team.';
  $replacement_keys = explode(',', '!weakest');
  $replacement_values = $dmo->get_data($data_args);
  $format_args = array_combine($replacement_keys, $replacement_values);
  
  $output = format_string($str, $format_args);
  
  die(var_dump($format_args));
  
}
function txp_test_nested_blocks() {
  
  //return txp_test_single_bar();
  
  //txp_test_array_sorting();
  //txp_test_scatterplot_chart();
  
  //$dmo = new DiagnosticDMO(1);
  //$dmo->get_comments(array('category_filter' => 'Common Purpose'));
  //
  //$output = $dmo->get_table(array('data_key' => 'team_mean_by_question_with_sum_and_mean', 'category_filter' => 'Willingness'));
  //die(var_dump($output));
}

function txp_test_array_sorting() {
  $dmo = new DiagnosticDMO(2);
  $output = $dmo->get_table(array('data_key' => 'category_question_responses_with_row_and_column_summaries', 'category_filter' => 'Willingness'));
  
  $pop_first = array_shift($output['footer'][1]);
  $sorted = $output['footer'][1];
  asort($sorted, SORT_NUMERIC);
  
  foreach ($output['footer'][1] as $key => $cell) {
    $output['footer'][1][$key] = $sorted[$key] + 1;
  }
  
  die(var_dump($output['footer'][1]));
}

function txp_test_real_paths() {
  $uri = "public://report_templates";
  echo '<strong>$uri = </strong> <em style="color:blue;">' . $uri . '</em><br />';
  echo '<strong>drupal_realpath($uri):</strong> <em style="color:blue;">' . drupal_realpath($uri) . '</em><br />'; // in this case, drupal_realpath takes a URI
  echo '<strong>file_create_url($uri):</strong> <em style="color:blue;">' . file_create_url($uri) . '</em><br />';
  echo '<br />';
  $pchart_library = libraries_get_path('pchart');
  echo "<strong>libraries_get_path('pchart'):</strong> <em style=\"color:blue;\">" . libraries_get_path('pchart') . '</em><br />';
  echo '<strong>$pchart_library = </strong> <em style="color:blue;">' . $pchart_library . '</em><br />';
  echo "<strong>file_build_uri($pchart_library):</strong> <em style=\"color:blue;\">" . file_build_uri(libraries_get_path('pchart')) . '</em><br />';
  echo "<strong>realpath($pchart_library):</strong> <em style=\"color:blue;\">" . realpath(libraries_get_path('pchart')) . '</em><br />';
  $pchart_library_uri = file_build_uri($pchart_library);
  echo $pchart_library_uri . '<br />';
  echo 'drupal_realpath: ' . drupal_realpath($pchart_library); // in this case, drupal_realpath takes a relative path ref.
  
  exit();

}

function txp_test_single_bar() {
  
  $report_css_file = dirname(__FILE__) . '/pdf.css';
  $style = new triaxia_CSSParser();
  $style->Parse($report_css_file);
  
  $datum = array('Mean' => array(3.7));
  
  $template_folder_uri = 'public://report_templates';
  $chart_args = array(
    'id'            => 'single_bar_barchart',
    'css_id'            => '#single-bar-barchart',
    'type'          => 'chart',
    'chart_type'    => 'bar',
    'bar_position'  => 'horizontal',
    'css'           => $style,
    'width'         => 300,
    'height'        => 30,
    'base_class'    => 'survey',
    'class'         => 'barchart',
    'image_folder_uri'  => $template_folder_uri,
  );
  
  $chart = PdfLibChartFactory::get_single_bar($chart_args, $datum);
  
  $items = array();
  $items['bar-chart'] = array(
      '#type' => 'markup',
      '#markup' => '<div>' . $chart->get_html() . '</div>',
      '#weight' => 15,
  );
  
  return $items;
}

function txp_test_some_chart_tests() {
  
  $report_css_file = dirname(__FILE__) . '/pdf.css';
  $style = new triaxia_CSSParser();
  $style->Parse($report_css_file);
  
  $dmo = new DiagnosticDMO(1);
  //die(var_dump($dmo->get_data()));
  
  $template_folder_uri = 'public://report_templates';
  //$image_folder = drupal_realpath($template_folder_uri);
  $block = array(
    'id'            => 'survey_overview_barchart',
    'type'          => 'chart',
    'chart_type'    => 'bar',
    'bar_position'  => 'horizontal',
    'data_key'      => 'category_means',
    'css'           => $style,
    'width'         => 300,
    'height'        => 300,
    'base_class'    => 'survey',
    'class'         => 'barchart',
    'image_folder_uri'  => $template_folder_uri,
  );
  $chart = PdfLibChartFactory::create_chart($block, $dmo);
  
  return '<div>' . $chart->get_absolute_path() . '</div>';
  
  $items = array();
  $items['bar-chart'] = array(
      '#type' => 'markup',
      '#markup' => '<div>' . $chart->get_html() . '</div>',
      '#weight' => 15,
  );
  
  return $items;

  die(var_dump($chart));
  
  return "<h2>$chart</h2>";

  $dmo = new DiagnosticDMO(1);
  $data = $dmo->get_data_series();
  die(var_dump($data));
  //$table_args = array(
  //  'data_key' => 'member_mean_by_category',
  //  'footer_keys'  => array('total_mean_by_category','rank_team_mean_by_category'),
  //);
  //$table = $dmo->get_table($table_args);
  //die(var_dump($table));
  
  //txp_test_fetch_responses(2);
  //txp_test_various_pdflib_tests();
  //exit();
  
  $items = array();
  $items['bar-chart'] = array(
      '#type' => 'markup',
      '#markup' => '<div>' . txp_test_render_barchart('HORIZONTAL') . '</div>',
      '#weight' => 15,
  );
  
  $items['scatter-plot'] = array(
      '#type' => 'markup',
      '#markup' => '<div>' . txp_test_scatterplot_chart() . '</div>',
      '#weight' => 10,
  );
  
  $items['radar-chart'] = array(
      '#type' => 'markup',
      '#markup' => '<div>' . txp_test_radar_chart() . '</div>',
      '#weight' => 5,
  );
  
  return $items;
  

}

function txp_test_array_handling() {
  $args = array(
      'row' => 'team', 
      'col' => 'category',
      'fn'  => 'mean',
      'category_filter' => 'Clear Roles',
      'row_label' => array('member_num' => 'Member'), 
      'row_summary' => array('mean', 'sum'),
  );
  
  
  $row_label = each($args['row_label']);
  die( var_dump($row_label['key'] . ':' . $row_label['value']) );
  
}

function txp_test_render_barchart($chart_type = 'VERTICAL') {
  
  //$factory = new PdfLibChartFactory();
  //$chart = $factory::create_chart($args);
  
  // 1. Dataset
  $data = new pData();
  $data->addPoints(array(3.88,4.05,3.83,3.98,2.00,4.02), 'Means');
  
    // set axis labels (optional)
  //$data->setAxisName(0, 'Team Average'); Set this if you want to display the Axis name on the chart
  $data->addPoints(array('Common Purpose','Clear Roles','Accepted Leadership','Effective Processes','Solid Relationships','Excellent Communication'), 'Categories');
  $data->setSerieDescription('Categories', 'Survey Categories');
  $data->setAbscissa('Categories');
  
  // set data colors
  $data->loadPalette(PCHART_PALETTE_DIRECTORY . 'evening.color', TRUE);
  
    // set axis position
  $axis_position = $chart_type == 'HORIZONTAL' ? AXIS_POSITION_RIGHT : AXIS_POSITION_LEFT;
  $data->setAxisPosition(0, $axis_position);  // if bar chart uses horizontal bars, set Axis to the "Right"
                                              // Confused? You should be. To render horizontal bars, pChart
                                              // turns the entire graph area clockwise 90¡ so the Y-Axis is
                                              // on the top and the X-Axis is on the left.

  // 2. Chart Object
  $image_width = 600; $image_height = 200;
  $transparent_background = FALSE;
  $image = new pImage($image_width, $image_height, $data, $transparent_background);
  $image->setShadow(TRUE,array("X"=>1,"Y"=>1,"R"=>0,"G"=>0,"B"=>0,"Alpha"=>40)); // (optional) if you want shadows on everything.
  
  // 3. Chart Area
  $chart_area_padding = array(10, 50, 0, 170); // use CSS padding syntax to set the Chart Area
  $top_left_x = $chart_area_padding[3];
  $top_left_y = $chart_area_padding[0];
  $bottom_right_y = $image_height - $chart_area_padding[2];
  $bottom_right_x = $image_width - $chart_area_padding[1];
  $image->setGraphArea($top_left_x, $top_left_y, $bottom_right_x, $bottom_right_y);
  
  // 4. Font
  $font_props = array(
    'FontName' => PCHART_FONT_DIRECTORY . 'verdana.ttf',
    'FontSize' => 10,
  );
  $image->setFontProperties($font_props);
  
  // 5. Scale
  $axis_boundaries = array(
    0 => array('Min' => 0,
               'Max' => 5),
  );
  $scale_settings = array(
    'Mode'        => SCALE_MODE_MANUAL,
    'ManualScale' => $axis_boundaries,
    'Factors'     => array(1), // scale units. display integers 1 - 5.
    'Pos'         => $chart_type == 'HORIZONTAL' ? SCALE_POS_TOPBOTTOM : SCALE_POS_LEFTRIGHT,
    'AxisAlpha'   => 0, // Set to zero if you don't want to see the axis lines
    'TickAlpha'   => 0, // "
    'GridAlpha'   => 0, // "
    //'RemoveXAxis' => TRUE,
  );
  $image->drawScale($scale_settings);
  
  // 6. Chart Type
  // (optional) if ya want, you can override the palette files here.
  // This will force each bar to be a different color.
  $alpha = 80;
  $palette = array(
      "0"=>array("R"=>188,"G"=>224,"B"=>46,"Alpha"=>$alpha),
      "1"=>array("R"=>224,"G"=>100,"B"=>46,"Alpha"=>$alpha),
      "2"=>array("R"=>224,"G"=>214,"B"=>46,"Alpha"=>$alpha),
      "3"=>array("R"=>46,"G"=>151,"B"=>224,"Alpha"=>$alpha),
      "4"=>array("R"=>176,"G"=>46,"B"=>224,"Alpha"=>$alpha),
      "5"=>array("R"=>224,"G"=>46,"B"=>117,"Alpha"=>$alpha),
      "6"=>array("R"=>92,"G"=>224,"B"=>46,"Alpha"=>$alpha),
  );
  $bar_chart_props = array(
    'DisplayValues' => TRUE,
    'DisplayPos'    => LABEL_POS_OUTSIDE, //LABEL_POS_INSIDE,
    'Rounded'       => TRUE,    // rounded bar corners
    'Interleave'    => 0.1,     // the space between the bars is equal to 1/10 the width of the bar
    'OverrideColors' => $palette,
    'Decimals' => 1,
  );
  $image->drawBarChart($bar_chart_props);
  
  // 7. Render
  $image_folder = variable_get('pchart_api_image_directory');
  $image_location = $image_folder . 'pdflib_barchar.png';
  $image->render($image_location);
  $chart = array(
    'path' => $image_location,
    'alt' => 'Bar Chart',
    'title' => 'pChart for PDFLib Test',
    'attributes' => array(),
  );
  return theme_image($chart);
  
}

function txp_test_scatterplot_chart() {
  // 1. DataSet
  $skill_data = txp_test_transform_scale_data(array(121,86,97,120,15,20,25,30));
  $willing_data = txp_test_transform_scale_data(array(115,109,91,119,150,150,150,135));
  $team_skill = txp_test_transform_scale_data(array(62.38));
  $team_will = txp_test_transform_scale_data(array(127.38));
  $data = new pData();
  
  // X-Axis
  $data->addPoints($skill_data, 'Skill');
  $data->addPoints($team_skill, 'Team Skill');
  $data->setAxisName(0, 'X-Axis');
  $data->setAxisXY(0, AXIS_X);
  $data->setAxisPosition(0, AXIS_POSITION_BOTTOM);
  
  // Y-Axis
  $data->addPoints($willing_data, 'Willingness');
  $data->addPoints($team_will, 'Team Willingness');
  $data->setSerieOnAxis('Willingness', 1);
  $data->setSerieOnAxis('Team Willingness', 1);
  $data->setAxisName(1, 'Y-Axis');
  $data->setAxisXY(1, AXIS_Y);
  $data->setAxisPosition(1, AXIS_POSITION_LEFT);
  
  // Bind 1st Data Series into Plot
  $data->setScatterSerie('Skill', 'Willingness', 0); // bind Skill-Willingness Member Scores
  $data->setScatterSerieDescription(0, 'Member Scores');
  $data->setScatterSerieColor(0, array('R' => 0, 'G' => 0, 'B' => 0));
  
  $data->setScatterSeriePicture(0, PCHART_RESOURCE_DIRECTORY . 'serie1.png');
  
  
  // Bind 2nd Data Series into Plot
  $data->setScatterSerie('Team Skill', 'Team Willingness', 1); // bind Skill-Willingness Team Scores
  $data->setScatterSerieDescription(1, 'Team Scores');
  $data->setScatterSerieColor(1, array('R' => 255, 'G' => 0, 'B' => 0));
  
  $data->setScatterSeriePicture(1, PCHART_RESOURCE_DIRECTORY . 'serie2.png');
  
  // Change Series Display Properties
  $data->setAxisColor(0,array("R"=>255,"G"=>255,"B"=>255));
  $data->setAxisColor(1,array("R"=>255,"G"=>255,"B"=>255));
  
  die(var_dump($data->Data));
  
  // 2. Chart Object
  $image_width = 520; $image_height = 520;
  $transparent_background = FALSE;
  $image = new pImage($image_width, $image_height, $data, $transparent_background);
  $image->drawFromJPG(0, 5, PCHART_RESOURCE_DIRECTORY . 'TeamCatChart_0-150.jpg');
  
  //$image->setShadow(TRUE,array("X"=>1,"Y"=>1,"R"=>0,"G"=>0,"B"=>0,"Alpha"=>40)); // (optional) if you want shadows on everything.
  
  // 3. Chart Area bottom = 22
  $chart_area_padding = array(7, 22, 25, 17); // use CSS padding syntax to set the Chart Area
  $top_left_x = $chart_area_padding[3];
  $top_left_y = $chart_area_padding[0];
  $bottom_right_y = $image_height - $chart_area_padding[2];
  $bottom_right_x = $image_width - $chart_area_padding[1];
  $image->setGraphArea($top_left_x, $top_left_y, $bottom_right_x, $bottom_right_y);
  
  // 4. Font
  $font_props = array(
    'FontName' => PCHART_FONT_DIRECTORY . 'verdana.ttf',
    'FontSize' => 10,
  );
  $image->setFontProperties($font_props);
  
  // 5. Scale
  $axis_boundaries = array(
    0 => array('Min' => 0,
               'Max' => 150),
    1 => array('Min' => 0,
               'Max' => 150),
  );
  $scale_settings = array(
    'Mode'        => SCALE_MODE_MANUAL,
    'ManualScale' => $axis_boundaries,
    'Factors'     => array(10), // scale units. display integers 1 - 5.
    'AxisAlpha'   => 0, // Set to zero if you don't want to see the axis lines
    'TickAlpha'   => 0, // "
    'GridR'       => 255,
    'GridG'       => 255,
    'GridB'       => 255,
    'GridAlpha'   => 0, // "
    'DrawXlines'  => TRUE,
    'DrawYLines'  => ALL,
  );
  
  $scatter = new pScatter($image, $data);
  $scatter->drawScatterScale($scale_settings);
  
  // 6. Chart Type
  $scatter->drawScatterPlotChart();
  
  // 7. Render
  $image_folder = variable_get('pchart_api_image_directory');
  $image_location = $image_folder . 'pdflib_scatterplot.png';
  $image->render($image_location);
  $chart = array(
    'path' => $image_location,
    'alt' => 'Scatter Plot',
    'title' => 'pChart for PDFLib Test',
    'attributes' => array(),
  );
  return theme_image($chart);
}

function txp_test_transform_scale_data($data = array()) {
  //TODO: Configure PDFLib Block with a custom property for X- and Y-Axis transformations
  // Use the graph-scale-transformations.xlsx spreadsheet to workout number transformations for chart scales
  $transformations = array(0,0.5,1,1.5,2,2.5,3,3.5,4,4.5,5,5.4,5.8,6.2,6.6,7,7.4,7.8,8.2,8.6,9,9.6,10.2,10.8,11.4,12,12.6,13.2,13.8,14.4,15,15.7,16.4,17.1,17.8,18.5,19.2,19.9,20.6,21.3,22,22.8,23.6,24.4,25.2,26,26.8,27.6,28.4,29.2,30,30.9,31.8,32.7,33.6,34.5,35.4,36.3,37.2,38.1,39,39.9,40.8,41.7,42.6,43.5,44.4,45.3,46.2,47.1,48,49.1,50.2,51.3,52.4,53.5,54.6,55.7,56.8,57.9,59,60,61,62,63,64,65,66,67,68,69,70.2,71.4,72.6,73.8,75,76.2,77.4,78.6,79.8,81,82.2,83.4,84.6,85.8,87,88.2,89.4,90.6,91.8,93,94.4,95.8,97.2,98.6,100,101.4,102.8,104.2,105.6,107,108.1,109.2,110.3,111.4,112.5,113.6,114.7,115.8,116.9,118,119.5,121,122.5,124,125.5,127,128.5,130,131.5,133,134.7,136.4,138.1,139.8,141.5,143.2,144.9,146.6,148.3,150);
  $MAX_IDX = count($transformations) - 1;
  $MAX = $transformations[$MAX_IDX];
  
  $transformed_data = array();
  foreach ($data as $datum) {
    $datum_int = round($datum);
    if ($datum_int >= count($transformations)) {
      $transformed_data[] = $MAX;
      continue;
    }
    
    $transformed_data[] = $transformations[$datum_int]; // each integer of the value in the dataset is the index in the transformation
  }
  return $transformed_data;
}

function txp_test_radar_chart() {

  // 1. DataSet*
  $averages = array(3.30,2.3,4.7);
  $categories = array('People', 'Purpose', 'Process');
  
  $data = new pData();
  $data->addPoints($averages, 'Mean');
  $data->setSerieDescription('Mean', 'Team Averages');
  
  $data->addPoints($categories, 'Categories');
  $data->setAbscissa('Categories');
  
  $series_settings = array("R"=>255,"G"=>255,"B"=>255,"BorderR"=>0,"BorderG"=>0,"BorderB"=>0,"Alpha"=>90);
  $data->setPalette('Mean', $series_settings);

  // 2. Chart Object (canvas)
  $image_width = 500; $image_height = 500;
  $image = new pImage($image_width, $image_height, $data);
  
  // 3. Chart Area bottom = 22
  $chart_area_padding = array(25, 25, 25, 25); // use CSS padding syntax to set the Chart Area
  $top_left_x = $chart_area_padding[3];
  $top_left_y = $chart_area_padding[0];
  $bottom_right_y = $image_height - $chart_area_padding[2];
  $bottom_right_x = $image_width - $chart_area_padding[1];
  $image->setGraphArea($top_left_x, $top_left_y, $bottom_right_x, $bottom_right_y);
  
  // 4. Font
  $font_props = array(
    'FontName' => PCHART_FONT_DIRECTORY . 'verdana.ttf',
    'FontSize' => 10,
    "R" => 0, "G" => 0, "B" => 0, 
    "Alpha" => 100,
  );
  $image->setFontProperties($font_props);
  
  // 5. Scale* -- Apparently, not needed for Radar Charts. Radar rolls its own.
  //$axis_boundaries = array(
  //  0 => array('Min' => 0,
  //             'Max' => 5),
  //);
  //$scale_settings = array(
  //  'Mode'        => SCALE_MODE_MANUAL,
  //  'ManualScale' => $axis_boundaries,
  //  'Factors'     => array(1), // scale units. display integers 1 - 5.
  //  //'Pos'         => $chart_type == 'HORIZONTAL' ? SCALE_POS_TOPBOTTOM : SCALE_POS_LEFTRIGHT,
  //  'AxisAlpha'   => 0, // Set to zero if you don't want to see the axis lines
  //  'TickAlpha'   => 0, // "
  //  'GridAlpha'   => 0, // "
  //  //'RemoveXAxis' => TRUE,
  //);
  //$image->drawScale($scale_settings);
  
  
  
  // 6. Chart Type*
  $radar = new pRadar();
  $options = array(
    "DrawPoly"=>TRUE,
    "WriteValues"=>TRUE,
    "ValueFontSize"=>8,
    "Layout"=>RADAR_LAYOUT_CIRCLE,
    "BackgroundGradient"=>array(
                                "StartR"=>255,
                                "StartG"=>255,
                                "StartB"=>255,
                                "StartAlpha"=>100,
                                "EndR"=>207,
                                "EndG"=>227,
                                "EndB"=>125,
                                "EndAlpha"=>50
                                )
  );
  
  $options = array(
            "AxisR" => 100, "AxisG" => 40, "AxisB" => 220, "AxisAlpha" => 100,
            "LabelsBGR" => 40, "LabelsBGG" => 40, "LabelsBGB" => 40, "LabelsBGAlpha" => 70,
            "AxisRotation" => 30,
            "SegmentHeight" => 1,
            "Segments" => 5,
            "DrawPoly"=>TRUE,
            "DrawPoints" => TRUE,
            "OuterBubbleRadius" => 6,
            "OuterBubbleR" => 0, "OuterBubbleG" => 46, "OuterBubbleB" => 184, "OuterBubbleAlpha" => 30,
            "InnerBubbleR" => 20, "InnerBubbleG" => 20, "InnerBubbleB" => 20, "InnerBubbleAlpha" => 20,
            "PolyAlpha" => 20,
            "WriteValues"=>TRUE,
            "ValueFontSize"=> 9,
            "ValuePadding" => 10,
            "WriteValuesInBubble" => TRUE,
            "LabelPos" => RADAR_LABELS_HORIZONTAL, // RADAR_LABELS_ROTATED, //  
            "Layout" => RADAR_LAYOUT_STAR, // RADAR_LAYOUT_CIRCLE, //
            //"BackgroundGradient" => array(
            //    "StartR"=>255,"StartG"=>255,"StartB"=>255,"StartAlpha"=>100,
            //    "EndR"=>207,"EndG"=>227,"EndB"=>125,"EndAlpha"=>50,
            //),
        );
  $radar->drawRadar($image,$data,$options); 

  
  // 7. Render
  $image_folder = variable_get('pchart_api_image_directory');
  $image_location = $image_folder . 'pdflib_radar.png';
  $image->render($image_location);
  $chart = array(
    'path' => $image_location,
    'alt' => 'Scatter Plot',
    'title' => 'pChart for PDFLib Test',
    'attributes' => array(),
  );
  return theme_image($chart);
  
}

function txp_test_various_pdflib_tests() {
  
  $report_css_file = dirname(__FILE__) . '/pdf.css';
  $style = new triaxia_CSSParser();
  $style->Parse($report_css_file);
  //
  //die(var_dump($style->css));

  //$output = $style->Get('no.survey-header', 'background-color');
  //return '<h2>Test Complete</h2><p>Output: ' . $output . '</p>';
  
  $report_info = get_team_diagnostic_info_from_line_item(1);
  $report_info['line_item_id'] = 1;
  $report_info['base_class'] = 'survey';
  $report_info['css'] = $style;
  
  $pdf = new DiagnosticPDF($report_info);
  //$pdf->merge_template('team_performance_survey_template.pdf'); // implement this arg in $report_info
  $pdf->merge_template('benchmark_diagnostic.pdf');
  $pdf->print_pdf();
  exit();
  
  return '<h3>Test Complete!</h3>';
  
  //$responses = txp_test_fetch_responses(1);
  //$table = txp_test_get_category_mean_by_member($responses);
  //$table = txp_test_get_team_mean_by_category($responses);
  return '<h2>Test Complete</h2>';
  //return parse_pdf_pages();
  //txp_test_table();
  
  // Tests to perform:
  // 1. Fill team name on title page -- DONE!
  
  $pdf = new DiagnosticPDF(array('creator' => 'Test Creator', 'title' => 'Team Diagnostic Test'));
  $pdf->merge_template('team_conflict_template.pdf');
  $pdf->print_pdf();
  exit();
  
}

function txp_test_fetch_responses($line_item_id) {
  
  $query = db_select('tptd_response_details', 'd');
  $query->innerJoin('tptd_survey_items', 'i', 'd.item_id = i.item_id');
  $query->innerJoin('tptd_response', 'r', 'd.response_id = r.response_id');
  
  $query->addField('r', 'response_id');
  $query->addField('r', 'team_member_email');
  $query->addField('i', 'item_id');
  $query->addField('i', 'item_type');
  $query->addField('i', 'item_text');
  $query->addField('i', 'category');
  $query->addField('i', 'delta');
  $query->addField('d', 'points');
  
  $query->condition('r.order_line_item_id', $line_item_id, '=');
  $query->condition('d.points', 0, '>');
  
  $result = $query->execute();
  $items = array();
  $responses = array();
  $participants = array();
  while ($record = $result->fetchAssoc()) {
    // 1. raw detail: $responses[] = $record;
    // 2. by response then item: $responses[$record['response_id']][$record['item_id']] = $record;
    // 3. by items
    
    $participants[$record['response_id']] = $record['team_member_email'];
    $responses[$record['delta']][$record['response_id']] = (float)$record['points'];
    //= array(
    //    'response_id' => $record['response_id'],
    //    'email' => $record['team_member_email'],
    //    'points' => $record['points'],
    //);
    $items[$record['delta']] = array(
      'id'        => $record['item_id'],
      'category' => $record['category'],
      'text' => $record['item_text'],
      'type' => $record['item_type'],
      'responses' => $responses[$record['delta']],
    );
    
    // 4. by category, item, with team in separate array.
  }
  
  $test = array(
    'team' => $participants,
    'points' => $items[1]['responses'],
    'sum' => array_sum($items[1]['responses']),
    'avg' => array_sum($items[1]['responses']) / count($items[1]['responses']),
  );
  
  die(var_dump($items));
  return $responses;
        
}

function txp_test_css_parser() {
  $report_css_file = dirname(__FILE__) . '/pdf.css';
  $style = new triaxia_CSSParser();
  $style->Parse($report_css_file);
  //die(var_dump($css->css));
  
  return '<h2>Parse Test Complete</h2><p>The background-color for table.survey-header is ' . $style->css['table.survey-header']['background-color'] . '</p>';
  
  
}

function txp_test_get_team_mean_by_category($response_details) {
    $footer = array();
    $footer['summary_label'] = t('Team Average');
    $categories = array();
    foreach ($response_details as $detail) {
        
        if (!in_array($detail['category'], $categories)) {
            $categories[] = $detail['category'];
        }
        
        $footer[$detail['category']][] = $detail['points'];
    }
    
    foreach ($categories as $category) {
        $cat_sum = array_sum($footer[$category]);
        $cat_count = count($footer[$category]);
        $footer[$category] = number_format($cat_sum / $cat_count, 2);
    }
    
    return $footer;
}

function txp_test_get_category_mean_by_member($response_details) {
    $headers = array();
    $headers[] = t('Member');
    $categories = array();
    
    $rows = array();
    foreach ($response_details as $detail) {
        // some local variables for easier reading...you're welcome.
        $response_id = $detail['response_id'];
        $category = $detail['category'];
        
        // load headers
        if (!in_array($category, $headers)) {
            $headers[] = $category;
            $categories[] = $category;
        }
        
        
        // initialize member row if it doesn't exist.
        if (!isset($rows[$response_id])) {
            $rows[$detail['response_id']] = array();
            $rows[$detail['response_id']]['member_num'] = '#' . $response_id;
        }
        
        // sum up category points for each member as code loops
        $rows[$response_id][$category][] = $detail['points'];
        
    }
    
    // calculate category means using the count array
    foreach ($rows as $member_id => $row) {
        foreach($categories as $category) {
          $cat_sum = array_sum($rows[$member_id][$category]);
          $cat_count = count($rows[$member_id][$category]);
          $rows[$member_id][$category] = number_format(($cat_sum / $cat_count), 2);
        }
    }
    
    return array('headers' => $headers, 'data' => $rows);
}

function get_absolute_path_to_image() {
  global $base_url;
  $image_path = drupal_realpath('public://pchart'); // . '/' . another_sample_chart();
  $image_path = realpath( another_sample_chart() );
  return $image_path;
}

function txp_test_query_block() {
  
}

function get_huge_block_of_text() {
  return 'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed neque nulla, adipiscing eget tincidunt et, ornare a elit. Aenean dolor velit, aliquam et fermentum id, convallis eu mi. Proin eleifend sapien suscipit tellus ultricies quis aliquet lacus aliquam. Curabitur eget rutrum nisi. Duis eu metus nulla. Aliquam nisl nibh, pretium ac dapibus eget, ultrices id diam. Integer sed velit arcu. Nulla ante arcu, lobortis quis commodo at, porttitor non nisi. Phasellus est metus, scelerisque quis interdum nec, suscipit id tellus. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Maecenas fermentum pharetra nisi sit amet interdum. Fusce tempus elit in massa sodales hendrerit quis eu diam. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Curabitur tempor, elit et luctus facilisis, risus nulla pharetra risus, vitae vehicula nunc nibh nec lacus. Praesent vitae erat quis est aliquam egestas sed ut turpis. In in arcu lorem, nec vestibulum neque. Donec cursus est mi, tincidunt commodo ante. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse dolor ipsum, fringilla vitae rutrum ut, pharetra nec erat. Aenean et interdum tellus. Proin blandit, arcu ut iaculis pulvinar, metus enim porttitor orci, vel pretium nisl risus sed arcu. Donec suscipit lobortis porttitor. Aenean mollis augue a neque tincidunt fermentum a sed turpis. Ut vitae neque quis metus tristique fermentum non nec metus. Praesent vitae bibendum odio. Quisque eros dolor, lacinia nec pulvinar id, pellentesque et nisl. Ut consequat pellentesque enim, eget dapibus justo sodales ac. Etiam auctor nulla eget elit dictum laoreet. Nullam pharetra adipiscing neque venenatis volutpat. Nam malesuada posuere laoreet. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam vel ligula purus. Sed eget arcu ac lectus fringilla sodales. Ut sed enim neque. Nullam vel velit sodales arcu fermentum rhoncus. Etiam fermentum lobortis turpis vel lobortis. Fusce bibendum sagittis justo porta tincidunt. Integer a elit ante, sed porta ipsum. Integer a faucibus nisl. Sed massa nulla, sodales ac pulvinar cursus, molestie non dui. Vivamus nec metus metus, id pulvinar enim. Fusce non semper neque. Cras dapibus sapien sit amet sem hendrerit in mollis urna suscipit. In facilisis dignissim arcu, ac pharetra metus mollis nec. Duis vehicula, odio vitae sollicitudin consectetur, metus augue congue ipsum, eu consequat ipsum arcu at nisl. Suspendisse neque odio, viverra at placerat et, laoreet ac ipsum. Pellentesque mi velit, viverra in ornare a, rhoncus quis eros. Sed vulputate leo quis nibh mattis at elementum justo fringilla. Morbi eros diam, aliquet ut ullamcorper non, sagittis ac velit. Aliquam nec ligula justo. Vivamus tempus egestas metus, sit amet sagittis velit ullamcorper id. Nullam et lorem dapibus metus tempor congue eget vitae diam. Suspendisse fermentum purus a nibh tincidunt dapibus. Nunc non enim vel lectus varius facilisis dignissim nec lacus.';
}

function txp_test_table() {
  $searchpath = drupal_realpath('public://report_templates');
  $outfilename = "";
  
  
  $rowmax = 35; $colmax = 5;
  $llx= 100; $lly=100; $urx=550; $ury=600;
  
  $headertext = "Table header (centered across all columns)";
  
  try {
      $p = new PDFlib();
  
      # This means we must check return values of load_font() etc.
      $p->set_parameter("errorpolicy", "return");
  
      $p->set_parameter("SearchPath", $searchpath);
  
      /* we use "bytes" as textformat, this allows to use unicode encoding */
      $p->set_parameter("textformat", "bytes");
  
      if ($p->begin_document($outfilename, "") == 0) {
          die("Error: " . $p->get_errmsg());
      }
  
      $p->set_info("Creator", "PDFlib starter sample");
      $p->set_info("Title", "starter_table");
      
      $pdi_doc = $p->open_pdi_document('team_conflict_template.pdf', '');
      if ($pdi_doc == 0) {
          set_drupal_message("Error: " . $p->get_errmsg(), 'error', FALSE);
          continue;
      }
      
      $endpage = $p->pcos_get_number($pdi_doc, "length:pages");
    
            /* Loop over all pages of the input document */
      for ($pageno = 2; $pageno <= $endpage; $pageno++) {
        
        $tbl=0;
        // obnoxiously, the "page" parameter for open_pdi_page() is not zero-based.
        $page = $p->open_pdi_page($pdi_doc, $pageno, "");

        if ($page == 0) {
            set_drupal_message("Error: " . $p->get_errmsg(), 'error', FALSE);
            continue;
        }
        
        /* Dummy $page size; will be adjusted later */
        $p->begin_page_ext(10, 10, "");
        // Place the imported $page on the output $page, and adjust the $page size
        $p->fit_pdi_page($page, 0, 0, "adjustpage");

  
      //$p->begin_page_ext(0, 0, "width=a4.width height=a4.height");
        /* -------------------- Add table cells -------------------- */
    
        /* ---------- row 1: table header (spans all columns) */
        $row = 1; $col = 1;
        $font = $p->load_font("Times-Bold", "unicode", "");
        if ($font == 0) {
            die("Error: " . $p->get_errmsg());
        }
    
        $optlist = "fittextline={position=center font=" . $font . " fontsize=14} " .
        "colspan=" . $colmax;
    
        $tbl = $p->add_table_cell($tbl, $col, $row, $headertext, $optlist);
        if ($tbl == 0) {
            die("Error: " . $p->get_errmsg());
        }
    
    
        /* ---------- Fill $row 2 and above with their numbers */
        for ($row++; $row <= $rowmax; $row++) {
            for ($col = 1; $col <= $colmax; $col++) {
                $num = "Col " . $col . "/Row " . $row;
                $optlist = "colwidth=20% fittextline={font=" . $font . " fontsize=10}";
                $tbl = $p->add_table_cell($tbl, $col, $row, $num, $optlist);
                if ($tbl == 0) {
                    die("Error: " . $p->get_errmsg());
                }
            }
        }
    
        /* ---------- Place the table on one or more pages ---------- */
    
        /*
         * Loop until all of the table is placed; create new pages
         * as long as more table instances need to be placed.
         */
    //    do {
    //	$p->begin_page_ext(0, 0, "width=a4.width height=a4.height");
    
            /* Shade every other $row; draw lines for all table cells.
             * Add "showcells showborder" to visualize cell borders 
             */
            $optlist = "header=1 fill={{area=rowodd fillcolor={gray 0.9}}} " .
            "stroke={{line=other}} ";
    
            /* Place the table instance */
            $result = $p->fit_table($tbl, $llx, $lly, $urx, $ury, $optlist);
            if ($result ==  "_error") {
                die("Couldn't place table: " . $p->get_errmsg());
            }
            
            $p->delete_table($tbl, "");
    
            $p->close_pdi_page($page);
            $p->end_page_ext("");
      }
  
  //    } while ($result == "_boxfull");
  
      
  
      /* This will also delete Textflow handles used in the table */
  //    $p->delete_table($tbl, "");
  
      $p->end_document("");
  
      $buf = $p->get_buffer();
      $len = strlen($buf);
  
      header("Content-type: application/pdf");
      header("Content-Length: $len");
      header("Content-Disposition: inline; filename=starter_table.pdf");
      print $buf;
  
  }
  catch (PDFlibException $e) {
      die("PDFlib exception occurred in starter_table sample:\n" .
          "[" . $e->get_errnum() . "] " . $e->get_apiname() . ": " .
          $e->get_errmsg() . "\n");
  }
  catch (Exception $e) {
      die($e);
  }
  
  $p = 0;
  exit();
}

function test_repeating_headers() {
          //TODO: Block of code below is for testing. REMOVE!
//        $tllx = $table['llx'];
//        $tlly = $table['lly'] - 15;
//        $page_width = $this->pdf_lib->pcos_get_number($page, 'pages[0]/width');
//        $page_height = $this->pdf_lib->pcos_get_number($page, 'pages[0]/height');
//        $text_opts = 'fontname={DejaVuSerif} encoding=unicode embedding fontsize=12 textformat=utf8';
//        //$this->pdf_lib->fit_textline('Page width: ' . $page_width . ' | x pos: ' . $tllx, $tllx, $tlly, $text_opts);
//        $tf = 0;
//        $text = get_huge_block_of_text();
//        $tf = $this->pdf_lib->add_textflow($tf, $text, $text_opts);
//        if ($tf == 0)
//	    die("Error: " . $this->pdf_lib->get_errmsg());
        
//        // coordinate of first textflow box below table.
//        $tfllx = 46;
//        $tflly = 46;
//        $tfurx = 566;
//        $tfury = $table['lly'] - 160;
//        $tf_optlist = "verticalalign=justify linespreadlimit=120% ";
//        $result = $this->pdf_lib->fit_textflow($tf, $tfllx, $tflly, $tfurx, $tfury, $tf_optlist);
//        
//        if ($result == "_boxfull" || $result == "_nextpage") {
//            //foreach ($this->repeated as $delta => $repeated_block) {
//            //   $this->fill_repeated($repeated_block);
//            //}
//            $this->do_repeated_blocks();
//            
//        //    $this->pdf_lib->close_pdi_page($page);
//            $this->pdf_lib->end_page_ext("");
//            $this->page_num++;
//            $this->page_closed = true;
//        }
//        
//        while($result == "_boxfull" || $result == "_nextpage") {
//            // extend textflow to next page
//            
//            //$this->pdf_lib->begin_page_ext(612, 792, ""); //TODO: set width and height as class variables
//            
//            $this->pdf_lib->begin_page_ext(0, 0, "");
//            $this->pdf_lib->fit_pdi_page($page, 0, 0, "cloneboxes");
//           
//            $result = $this->pdf_lib->fit_textflow($tf, 46, 46, 566, 746, $tf_optlist);
//            //foreach ($this->repeated as $delta => $repeated_block) {
//            //    $this->fill_repeated($repeated_block);
//            //}
//            $this->do_repeated_blocks();
//            
//            $this->pdf_lib->end_page_ext("");
//            $this->page_num++;
//        }
//        
//        /* Check for errors */
//        if (!$result == "_stop") {
//            /* "_boxempty" happens if the box is very small and doesn't
//             * hold any text at all.
//             */
//            if ($result == "_boxempty") {
//                die("Error: Textflow box too small");
//            } else {
//                /* Any other return value is a user exit caused by
//                 * the "return" option; this requires dedicated code to
//                 * deal with.
//                 */
//                die("User return '" . $result . "' found in Textflow");
//            }
//       }
//        
//        $this->pdf_lib->delete_textflow($tf);
        // END OF TEST BLOCK

}

function _add_table_to_pdf() {
  
  $outfilename = '';
  
  $searchpath = drupal_realpath('public://report_templates');
  $pdf_template = "team_conflict_template.pdf";
  
  try {
    $p = new PDFlib();


    # This means we must check return values of load_font() etc.
    $p->set_parameter("errorpolicy", "return");

    $p->set_parameter("SearchPath", $searchpath);

    if ($p->begin_document($outfilename, "") == 0)
	die("Error: " . $p->get_errmsg());

    $p->set_info("Creator", "PDFlib Benchmark");
    $p->set_info("Title", "benchmark_pdfmerge_test");
    
    $indoc = $p->open_pdi_document($pdf_template, "");
    if ($indoc == 0) {
        printf("Error: %s\n", $p->get_errmsg());
        continue;
    }

    
    $page = $p->open_pdi_page($indoc, 1, "");
    
    $optlist = "encoding=winansi embedding";
    
    $p->begin_page_ext(10, 10, "");
    $p->fit_pdi_page($page, 0, 0, "adjustpage");
    
    $block_name = $p->pcos_get_string($indoc, 'pages[0]/PieceInfo/PDFlib/Private/Blocks[0]/Name');
    $p->fill_textblock($page, $block_name, 'Test Dynamic Fill', $optlist);
    
    $p->close_pdi_page($page);
    $p->end_page_ext("");

    $p->close_pdi_document($indoc);
    $p->end_document("");
    
    $buf = $p->get_buffer();
    $len = strlen($buf);

    header("Content-type: application/pdf");
    header("Content-Length: $len");
    header("Content-Disposition: inline; filename=benchmark_pdfmerge_test.pdf");
    print $buf;

    
  }
  catch (PDFlibException $e) {
    die("PDFlib exception occurred in benchmark_pdfmerge_test sample:\n" .
	"[" . $e->get_errnum() . "] " . $e->get_apiname() . ": " .
	$e->get_errmsg() . "\n");
  }
  catch (Exception $e) {
      die($e);
  }
  
  $p = 0;
  
  exit();
}

/*
 private function _txp_test_fill_table($page, $block) {
        $tbl=0;
        $rowmax = 10; $colmax = 5;
        $llx= 50; $lly=50; $urx=550; $ury=600;
        
        $headertext = "Table header (centered across all columns)";

        $row = 1; $col = 1;
        $font = $this->pdf_lib->load_font("Times-Bold", "unicode", "");
        if ($font == 0) {
            die("Error: " . $this->pdf_lib->get_errmsg());
        }
    
        $optlist = "fittextline={position=center font=" . $font . " fontsize=14} " .
        "colspan=" . $colmax;
    
        $tbl = $this->pdf_lib->add_table_cell($tbl, $col, $row, $headertext, $optlist);
        if ($tbl == 0) {
            die("Error: " . $this->pdf_lib->get_errmsg());
        }
    
    
        //---------- Fill $row 2 and above with their numbers 
        for ($row++; $row <= $rowmax; $row++) {
            for ($col = 1; $col <= $colmax; $col++) {
                $num = "Col " . $col . "/Row " . $row;
                $optlist = "colwidth=20% fittextline={font=" . $font . " fontsize=10}";
                $tbl = $this->pdf_lib->add_table_cell($tbl, $col, $row, $num, $optlist);
                if ($tbl == 0) {
                    die("Error: " . $this->pdf_lib->get_errmsg());
                }
            }
        }
        
        // Shade every other $row; draw lines for all table cells.
	// Add "showcells showborder" to visualize cell borders 
	 
	$optlist = "header=1 fill={{area=rowodd fillcolor={gray 0.9}}} " .
	"stroke={{line=other}} ";

	// Place the table instance 
	$result = $this->pdf_lib->fit_table($tbl, $llx, $lly, $urx, $ury, $optlist);
	if ($result ==  "_error") {
	    die("Couldn't place table: " . $this->pdf_lib->get_errmsg());
	}
        
        $this->pdf_lib->delete_table($tbl, "");
    }
    
*/

function parse_pdf_pages() {
  
  $outfilename = '';
  $title = 'Template Attributes';
  $type = 'ul';
  $items = array();
  $attributes = array();
  
  $searchpath = drupal_realpath('public://report_templates');
  $pdf_template = "team_conflict_template.pdf";
  
  try {
    $p = new PDFlib();


    # This means we must check return values of load_font() etc.
    $p->set_parameter("errorpolicy", "return");

    $p->set_parameter("SearchPath", $searchpath);

    if ($p->begin_document($outfilename, "") == 0)
	die("Error: " . $p->get_errmsg());

    $p->set_info("Creator", "PDFlib Benchmark");
    $p->set_info("Title", "benchmark_pdfmerge_test");
    
    $indoc = $p->open_pdi_document($pdf_template, "");
    if ($indoc == 0) {
        printf("Error: %s\n", $p->get_errmsg());
        continue;
    }

    $items[] = 'Pages: ' . $p->pcos_get_number($indoc, "length:pages");
    
    $page = $p->open_pdi_page($indoc, 1, "");
    
    // parse the template
    $items[] = 'length:pages[0]/PieceInfo/PDFlib/Private/Blocks: ' . $p->pcos_get_number($indoc, 'length:pages[0]/PieceInfo/PDFlib/Private/Blocks');
    $items[] = 'pages[0]/PieceInfo/PDFlib/Private/Blocks[0]/Name: ' . $p->pcos_get_string($indoc, 'pages[0]/PieceInfo/PDFlib/Private/Blocks[0]/Name');
    $items[] = 'type:pages[0]/blocks/title_page_team_name/textflow: ' . $p->pcos_get_string($indoc, 'type:pages[0]/blocks/title_page_team_name/textflow');
    //$items[] = 'pages[0]/blocks/title_page_team_name/leading: ' . $p->pcos_get_string($indoc, 'pages[0]/blocks/title_page_team_name/leading');
    $items[] = 'pages[0]/blocks/title_page_team_name/fontname: ' . $p->pcos_get_string($indoc, 'pages[0]/blocks/title_page_team_name/fontname');
    $items[] = 'pages[0]/blocks/title_page_team_name/fontsize: ' . $p->pcos_get_number($indoc, 'pages[0]/blocks/title_page_team_name/fontsize');
    //$items[] = 'pages[0]/blocks/title_page_team_name/fontstyle: ' . $p->pcos_get_string($indoc, 'pages[0]/blocks/title_page_team_name/fontstyle');
    $items[] = 'pages[0]/blocks/title_page_team_name/Rect[0]: ' . $p->pcos_get_number($indoc, 'pages[0]/blocks/title_page_team_name/Rect[0]');
    $items[] = 'pages[0]/blocks/title_page_team_name/backgroundcolor[0]: ' . $p->pcos_get_string($indoc, 'pages[0]/blocks/title_page_team_name/backgroundcolor[0]');
    $items[] = 'pages[2]/blocks/team_table_of_scores/backgroundcolor[0]: FAILS if no background set!!'; //. $p->pcos_get_number($indoc, 'pages[2]/blocks/team_table_of_scores/Rect[0]'); //backgroundcolor[0]');
    
    $is_textflow = $p->pcos_get_string($indoc, 'type:pages[2]/blocks/team_table_of_scores/textflow') == 'boolean';
    if ($is_textflow) {
      //$items[] = 'pages[2]/blocks/team_table_of_scores/alignment: '. $p->pcos_get_string($indoc, 'pages[2]/blocks/team_table_of_scores/alignment');
    }
    //$items[] = 'pages[0]/PieceInfo/PDFlib/Private/Blocks[0]/backgroundcolor: ' . $p->pcos_get_string($indoc, 'pages[1]/PieceInfo/PDFlib/Private/Blocks[0]/backgroundcolor[0]');
    /*
    $items[] = 'Blocks (Page 2): ' . $p->pcos_get_number($indoc, 'length:pages[1]/PieceInfo/PDFlib/Private/Blocks');
    $items[] = 'Blocks (Block 1): ' . $p->pcos_get_string($indoc, 'pages[1]/PieceInfo/PDFlib/Private/Blocks[0]/Name');
    $items[] = 'Blocks (Block Custom Property): ' . $p->pcos_get_string($indoc, 'pages[1]/PieceInfo/PDFlib/Private/Blocks[1]/Custom[0].key');
    $items[] = 'Blocks (Block Custom Property Value): ' . $p->pcos_get_string($indoc, 'pages[1]/PieceInfo/PDFlib/Private/Blocks[1]/Custom[0].val');
    */
    
    //$items[] = 'Blocks (Page 3): ' . $p->pcos_get_number($indoc, 'length:pages[3]/PieceInfo/PDFlib/Private/Blocks');
    //$items[] = 'Blocks (Page 4): ' . $p->pcos_get_number($indoc, 'length:pages[4]/PieceInfo/PDFlib/Private/Blocks');

    $p->close_pdi_page($page);
    $p->close_pdi_document($indoc);
    //$p->end_document("");
    
  }
  catch (PDFlibException $e) {
    die("PDFlib exception occurred in benchmark_pdfmerge_test sample:\n" .
	"[" . $e->get_errnum() . "] " . $e->get_apiname() . ": " .
	$e->get_errmsg() . "\n");
  }
  catch (Exception $e) {
      die($e);
  }
  
  $p = 0;
  
  return theme_item_list(array('items' => $items, 'title' => $title, 'type' => $type, 'attributes' => $attributes));
}

function create_mergepdf_nested() {
  
  $searchpath = drupal_realpath('public://report_templates');
  $outfilename = '';
  
  $pdffiles = array(
    "nested-block-test-template.pdf",
  );

  try {
    $p = new PDFlib();


    # This means we must check return values of load_font() etc.
    $p->set_parameter("errorpolicy", "return");

    $p->set_parameter("SearchPath", $searchpath);

    if ($p->begin_document($outfilename, "") == 0)
	die("Error: " . $p->get_errmsg());

    $p->set_info("Creator", "PDFlib Benchmark");
    $p->set_info("Title", "benchmark_nested_block_test");

    foreach ($pdffiles as $pdffile) { 
	/* Open the input PDF */
	$indoc = $p->open_pdi_document($pdffile, "");
	if ($indoc == 0) {
	    printf("Error: %s\n", $p->get_errmsg());
	    continue;
	}

	$endpage = $p->pcos_get_number($indoc, "length:pages");

	/* Loop over all pages of the input document */
	for ($pageno = 1; $pageno <= $endpage; $pageno++) {
            $page_index = $pageno - 1;
            
            // obnoxiously, the "page" parameter for open_pdi_page() is not zero-based.
	    $page = $p->open_pdi_page($indoc, $pageno, "");

	    if ($page == 0) {
		printf("Error: %s\n", $p->get_errmsg());
		continue;
	    }
	    /* Dummy $page size; will be adjusted later */
	    $p->begin_page_ext(10, 10, "");

	    /* Create a bookmark with the file name */
	    if ($pageno == 1) {
		$p->create_bookmark($pdffile, "");
	    }

	    /* Place the imported $page on the output $page, and
	     * adjust the $page size
	     */
	    $p->fit_pdi_page($page, 0, 0, "adjustpage");
            
            /* Option list for text blocks */
            $txtoptlist = "encoding=winansi embedding";
            
            $blockcount = $p->pcos_get_number($indoc, 'length:pages[' . $page_index . ']/PieceInfo/PDFlib/Private/Blocks');
            
            /* Fill the "announcement" block
            if ($p->fill_textblock($page, "title_page_team_name", 'Test Team Name', $txtoptlist) == 0) {
              drupal_set_message($p->get_errmsg(), 'error', FALSE);
            }
            */
            for($block_index = 0; $block_index < $blockcount; $block_index++) {
              $blockname = $p->pcos_get_string($indoc, 'pages[' . $page_index . ']/PieceInfo/PDFlib/Private/Blocks[' . $block_index . ']/Name');
              
              //if ($p->fill_textblock($page, $blockname, 'Test Block Text', $txtoptlist) == 0) {
              //  drupal_set_message($p->get_errmsg(), 'error', FALSE);
              //}
              
              $comments_pdi = $p->open_pdi_document('comments-block.pdf', "");
              if ($comments_pdi == 0)
                  die("Error: " . $p->get_apiname() . ": " . $p->get_errmsg());
          
              /* Open the first page */
              $comment_page = $p->open_pdi_page($comments_pdi, 1, "");
              if ($comment_page == 0)
                  die("Error: " . $p->get_apiname() . ": " . $p->get_errmsg());
                  
              
              /*
                * Imposition: fill first-level PDF Blocks with second-level
                * container page
                */
               if ($p->fill_pdfblock($indoc, $blockname, $comment_page, "") == 0) {
                   die("Warning: " . $p->get_errmsg());
               }
               
               if ($p->fill_textblock($comment_page, 'question_item', 'Question Text Goes here', $txtoptlist) == 0) {
                    die("Warning: " . $p->get_errmsg());
                }

              
            }
            
	    $p->close_pdi_page($page);

	    $p->end_page_ext("");
	}
	$p->close_pdi_document($indoc);
    }

    $p->end_document("");

    $buf = $p->get_buffer();
    $len = strlen($buf);

    header("Content-type: application/pdf");
    header("Content-Length: $len");
    header("Content-Disposition: inline; filename=benchmark_pdfmerge_test.pdf");
    print $buf;

}
catch (PDFlibException $e) {
    die("PDFlib exception occurred in benchmark_pdfmerge_test sample:\n" .
	"[" . $e->get_errnum() . "] " . $e->get_apiname() . ": " .
	$e->get_errmsg() . "\n");
}
catch (Exception $e) {
    die($e);
}

$p = 0;
exit();
}

function create_pdf() {
$x = 10;
$xt = 280;
$y = 800;
$yoff = 50;
$textline = "Triaxia Partners Team Benchamrk";
$searchpath = file_create_url('public://report_templates');

try {
    # create a new PDFlib object
    $p = new PDFlib();

    $p->set_parameter("SearchPath", $searchpath);

    # This means we must check return values of load_font() etc.
    $p->set_parameter("errorpolicy", "return");

    # Set an output path according to the name of the topic
    if ($p->begin_document("", "") == 0) {
	die("Error: " .  $p->get_errmsg());
    }

    $p->set_info("Creator", "PDFlib Cookbook");
    $p->set_info("Title", "Starter Text Line");

    # Start Page
    $p->begin_page_ext(0, 0, "width=a4.width height=a4.height");

    $font = $p->load_font("Helvetica", "winansi", "");

    if ($font == 0) {
	die("Error: " .  $p->get_errmsg());
    }

    # Set the font with a font size of 14
    $p->setfont($font, 14);


    # Place the text line without any options applied
    $p->fit_textline($textline, $x, $y, "");

    # Output descriptive text
    $p->fit_textline("fit_textline() without any options", $xt, $y,
	"fontsize=12");


    # Place the text with a different font size
    $optlist = "fontsize=22";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist); # sample text
    $p->fit_textline($optlist, $xt, $y, "fontsize=12"); # description
    
    
    # Place stroke out text
    $optlist = "strikeout";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist); # sample text
    $p->fit_textline($optlist, $xt, $y, "fontsize=12"); # description
    
    
    # Place underlined text
    $optlist = "underline underlinewidth=7% underlineposition=-20%";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist); # sample text
    $p->fit_textline($optlist, $xt, $y, "fontsize=12"); # description
    
    
    # Place overlined text
    $optlist = "overline";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist); # sample text
    $p->fit_textline($optlist, $xt, $y, "fontsize=12"); # description
    
    
    # Place the text with a horizontal scaling of 150%
    $optlist = "horizscaling=150%";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist); # sample text
    $p->fit_textline($optlist, $xt, $y, "fontsize=12"); # description
    
    
    # Place the text with a character spacing of 30% of the font size
    $optlist = "charspacing=30%";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist); # sample text
    $p->fit_textline($optlist, $xt, $y, "fontsize=12"); # description
    
    
    # Place the text with a word spacing of 50% of the font size
    $optlist = "wordspacing=50%";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist); # sample text
    $p->fit_textline($optlist, $xt, $y, "fontsize=12"); # description
    
    
    # Place the text with a different fill color
    $optlist = "fillcolor={rgb 0.5 0.2 0.5}";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist);
    $p->fit_textline($optlist, $xt, $y, "fontsize=12");
    
    
    # Place the text including its outlines using a text rendering mode of
    # 2 for "filling and stroking text" and different fill and stroke
    # colors
    
    $optlist = 
	"fontsize=22 fillcolor={rgb 0.6 0.3 0.6} strokecolor={gray 0} " .
	"strokewidth=0.4 textrendering=2";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist);
    
    # Output descriptive text
    $p->fit_textline("fillcolor={rgb 0.6 0.3 0.6} strokecolor={gray 0} ",
	$xt, $y+10, "fontsize=12");
    $p->fit_textline("strokewidth=0.4 textrendering=2 fontsize=22",
	$xt, $y-5, "fontsize=12");
    
    
    # Place the text with its outlines using a text rendering mode of
    # 1 for "stroking text" and a stroke color of black
    
    $optlist =
	"fontsize=22 strokecolor={gray 0} strokewidth=0.4 textrendering=1";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist);
    
    # Output descriptive text
    $p->fit_textline("strokecolor={gray 0} strokewidth=0.4", $xt, $y+10,
	"fontsize=12");
    $p->fit_textline("textrendering=1 fontsize=22", $xt, $y-=5,
	"fontsize=12");
    
    
    # Place the text in a box with default positioning and fitting
    $optlist = "boxsize={200 20} showborder";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist);   # sample text
    $p->fit_textline($optlist, $xt, $y+3, "fontsize=12"); # description
    
    
    # Place the text in a box on the top right
    $optlist = "boxsize={200 20} position={top right} showborder";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist);   # sample text
    $p->fit_textline($optlist, $xt, $y+3, "fontsize=12"); # description
    
    
    # Use "fitmethod=clip" to place the text in a box not large enough to 
    # show the complete text. The text will be clipped.
    
    $optlist = "boxsize={130 20} fitmethod=clip showborder";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist);   # sample text
    $p->fit_textline($optlist, $xt, $y+3, "fontsize=12"); # description
    
    
    # Fit the text into the box automatically with "fitmethod=auto".
    # In this case, if the text doesn't fit into the box a distortion
    # factor is calculated which makes the text fit into the box. If this 
    # factor is larger than the "shrinklimit" option the text will
    # be distorted by that factor. Otherwise, the font size will be
    # be decreased until until the text fits into the box.
    
		
    # Use "fitmethod=auto" to place the text in a box not large enough to 
    # show the complete text. The text will be distorted.
    
    $optlist = "boxsize={130 20} fitmethod=auto showborder";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist);   # sample text
    $p->fit_textline($optlist, $xt, $y+3, "fontsize=12"); # description
 
    
    # Use "fitmethod=auto" to place the text in a box too small to show the
    # complete text. The font size will be reduced until the text fits into
    # the box.
    
    $optlist = "boxsize={100 20} fitmethod=auto showborder";
    
    $p->fit_textline($textline, $x, $y-=$yoff, $optlist);   # sample text
    $p->fit_textline($optlist, $xt, $y+3, "fontsize=12"); # description

    $p->end_page_ext("");

    $p->end_document("");

    $buf = $p->get_buffer();
    $len = strlen($buf);

    header("Content-type: application/pdf");
    header("Content-Length: $len");
    header("Content-Disposition: inline; filename=starter_textline.pdf");
    print $buf;
}
catch (PDFlibException $e) {
    die("PDFlib exception occurred in starter_textline sample:\n" .
        "[" . $e->get_errnum() . "] " . $e->get_apiname() . ": " .
        $e->get_errmsg() . "\n");
}
catch (Exception $e) {
    die($e);
}

$p = 0;
exit();
}

function test_render_items() {
  $build = array(
    'test_markup' => array(
      '#type' => 'markup',
      '#markup' => '<div><h2>Hello Item</h2></div>',
    ),
    'test_list' => array(
      '#theme' => 'item_list',
      '#type' => 'ol',
      '#items' => array('Mares eat oats and does eat oats and little lambs eat ivy', 'Who let the dogs out?'),
      '#title' => 'Comments',
    ),
  );
  
  $test_output = drupal_render($build);
  
  return $build;
}

function get_status_pie() {
  $items = array();
  /*
  $chart = array(
    '#chart_id' => 'test_scatterplot',
    '#title' => chart_title(t('Team Survey'), '000000', 12),
    '#type' => CHART_TYPE_PIE_3D, //CHART_TYPE_PIE_3D, //
    '#size' => chart_size(100, 100),
    '#scale_range' => array(),
  );
  */
  $response = get_response_status(12);
  
  foreach($response as $delta => $diagnostic) {
    //$status['items'][] = $diagnostic['diagnostic_name'];
    
    $chart_id = 'chart_' . $delta;
    
    $chart = array(
      '#chart_id' => $chart_id,
      '#type' => CHART_TYPE_PIE_3D, //CHART_TYPE_PIE_3D, //
      '#size' => chart_size(110, 100),
      '#scale_range' => array(),
    );
    
    $chart['#title'] = chart_title($diagnostic['diagnostic_name'], '000000', 11);
    $unfinished = (int) $diagnostic['total_members'] - (int) $diagnostic['total_responses'];
    $finished = (int) (int) $diagnostic['total_responses'];
    $chart['#data'] = array($finished,$unfinished);
    
    
    $content = theme_chart(array('#chart_id' => $chart_id, 'chart' => $chart));
    
    $items[] = array(
      '#type' => 'markup',
      '#markup' => "<div>$content</div>",
    );
    
    $chart = array();
  }
  
  return $items;
}

function get_skill_will_chart() {
   $chart = array(
    '#chart_id' => 'test_scatterplot',
    '#title' => chart_title(t('Skill Willingness'), '3333ff', 22),
    '#type' => CHART_TYPE_SCATTER, //CHART_TYPE_PIE_3D, //
    '#size' => chart_size(600, 400), //array('#width' => 840, '#height' => 480),
  );
  
  /*
  $chart['#data'] = array(
    'skill' => array(10,10,10,10,10), // x-axis
    'willingness' => array(10,20,30,40,50), // y-axis
  );
  */
  $sql = "SELECT d.response_id, sum(if(i.category = 'Skill', d.points, 0)) AS skill, sum(if(i.category = 'Willingness', d.points, 0)) AS willingness
FROM tptd_survey_items i
 INNER JOIN tptd_response_details d ON i.item_id = d.item_id
 INNER JOIN tptd_response r ON d.response_id = r.response_id
WHERE i.diagnostic_id = 2 AND r.order_line_item_id = :line_item_id
GROUP BY d.response_id";
  $param_array = array(':line_item_id' => 2);
  
  $skill_will_data = get_array_from_sql($sql, $param_array);
  
  $scatter_points = array(
    'skill' => array(),
    'willingness' => array(),
  );
  foreach($skill_will_data as $delta => $row) {
    $scatter_points['skill'][] = $row['skill'];
    $scatter_points['willingness'][] = $row['willingness'];
  }
  $chart['#scale_range'] = array(0,150);
  $chart['#data'] = $scatter_points;
  
  return theme_chart(array('chart' => $chart));
}
function get_category_chart() {
  
  $chart = array(
    '#chart_id' => 'test_chart',
    '#title' => '', //chart_title(t('Team Survey Overview'), '3333ff', 22),
    '#type' => CHART_TYPE_BAR_H, //CHART_TYPE_PIE_3D, //
    '#size' => chart_size(605, 200), //array('#width' => 840, '#height' => 480),
    //'#grid_lines' => chart_grid_lines(30, 15),
    //'#bar_size' => chart_bar_size(100, 25),
  );
  
  
  $category_means = get_team_survey_category_means(1); // pass line item id
  foreach($category_means['mean_score'] as $delta => $mean) {
    $chart['#data'][] = $mean;
  }
  
  for($i = 0; $i < 6; $i++) {
    $chart['#mixed_axis_labels'][CHART_AXIS_Y_LEFT][0][] = chart_mixed_axis_label(t($category_means['category'][$i]));
    $chart['#mixed_axis_labels'][CHART_AXIS_X_BOTTOM][0][] = chart_mixed_axis_label($i);
  }
  
  //$chart['#mixed_axis_labels'][CHART_AXIS_X_BOTTOM][0][] = chart_mixed_axis_range_label(0, 5);
  
  
  $chart['#scale_range'] = array(0,5);
  $chart['#data_colors'] = array('A74324|462F57|87A093|506E85|D2862E|80072B');
  $chart['#chart_fill'] = array('bg', 's', '65432111');
  
  //return $chart;
  return theme_chart(array('chart' => $chart));
  
}

/**
 * This function loads report pages from a report header and outputs PDF.
 */
function get_report_nodes() {
  $report = node_load(15);
  $report_nids = $report->field_report_pages[$report->language];
  
  createReportPDF($report_nids);
  
  //HTML test
  $output = '<h2>Test Page</h2>';
  foreach($report_nids as $delta => $nid) {
    $output .= get_report_content($nid['nid']);
  }
  
  return $output;
}

function createReportPDF($report_nids) {
    require_once(libraries_get_path('tcpdf') . '/config/lang/eng.php');
    require_once(libraries_get_path('tcpdf') . '/tcpdf.php');

    $pdf = new TCPDF(PDF_PAGE_ORIENTATION, PDF_UNIT, PDF_PAGE_FORMAT, true, 'UTF-8', false);

    // set document information
    $pdf->SetCreator(PDF_CREATOR);
    $pdf->SetAuthor('Marc Porlier');
    $pdf->SetTitle('TCPDF Example for Team Benchmark');
    $pdf->SetSubject('Team Benchmark Reporting');
    $pdf->SetKeywords('Team Benchmark, Diagnostics, Team');
    
    // set default header data
    $pdf->SetHeaderData(PDF_HEADER_LOGO, PDF_HEADER_LOGO_WIDTH, 'Triaxia Diagnostic Test', PDF_HEADER_STRING);
    
    // set header and footer fonts
    $pdf->setHeaderFont(Array(PDF_FONT_NAME_MAIN, '', PDF_FONT_SIZE_MAIN));
    $pdf->setFooterFont(Array(PDF_FONT_NAME_DATA, '', PDF_FONT_SIZE_DATA));
    
    // set default monospaced font
    $pdf->SetDefaultMonospacedFont(PDF_FONT_MONOSPACED);
    
    //set margins
    $pdf->SetMargins(PDF_MARGIN_LEFT, PDF_MARGIN_TOP, PDF_MARGIN_RIGHT);
    $pdf->SetHeaderMargin(PDF_MARGIN_HEADER);
    $pdf->SetFooterMargin(PDF_MARGIN_FOOTER);
    
    //set auto page breaks
    $pdf->SetAutoPageBreak(TRUE, PDF_MARGIN_BOTTOM);
    
    //set image scale factor
    $pdf->setImageScale(PDF_IMAGE_SCALE_RATIO);
    
    //set some language-dependent strings
    $pdf->setLanguageArray($l);
    
    // ---------------------------------------------------------
    
    // set default font subsetting mode
    $pdf->setFontSubsetting(true);
    
    // Set font
    // dejavusans is a UTF-8 Unicode font, if you only need to
    // print standard ASCII chars, you can use core fonts like
    // helvetica or times to reduce file size.
    $pdf->SetFont('dejavusans', '', 14, '', true);
    
    // Add pages
    foreach($report_nids as $delta => $nid) {
      // This method has several options, check the source code documentation for more information.
      $pdf->AddPage();
      
      
      // Set some content to print
      $html = get_report_content($nid);
      // Print text using writeHTMLCell()
      
      $pdf->writeHTMLCell($w=0, $h=0, $x='', $y='', $html, $border=0, $ln=1, $fill=0, $reseth=true, $align='', $autopadding=true);
    }
    
    // ---------------------------------------------------------
    
    // Close and output PDF document
    // This method has several options, check the source code documentation for more information.
    $pdf->Output('diagnostic_report_001.pdf', 'I');
    exit();
}

function get_report_content($nid) {
  $page_node = node_load($nid);
  $html = $page_node->body[$page_node->language][0]['value'];
  
  $output = preg_replace('/[\n|\r]/', ' ', $html); //$html; //
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
    $pattern = '/<triaxia.*?id=[\'|"]' . $id . '[\'|"].*?>.*?<\\/triaxia>/i';
    
    // do some processing to create an HTML element from the xml node
    $new_element = '<div style="display:none">WhooHoo!</div>';
    switch($type) {
      case 'datagrid':
        $new_element = render_datagrid($el);
        break;
      case 'chart':
        $new_element = render_chart($el);  // "<div id='$id' style='background-color:red'>$id will go here</div>"; // 
        break;
    }
    // and then replace in back in the content string
    $output = preg_replace($pattern, $new_element, $output);
  }
  
  return $output;
}

function get_test_content_2() {
  // this test mimics a mixed node containing HTML markup and XML elements
  /*
  $string = "<p>Forty What?</p>
    <div style='float:right'>Joe</div>
    <p>Jane</p>
    <triaxia id='myGoogleChart1' type='chart'><sql>SELECT * FROM tptd_response_details</sql></triaxia>
    <div>
     I know that's the answer -- but what's the question?
    </div>
    <triaxia id='myDataGrid' type='datagrid'><sql>SELECT d.response_id as id, SUM(if(d.item_id = 1, d.points, 0)) as Q1, SUM(if(d.item_id = 7, d.points, 0)) as Q7, SUM(if(d.item_id = 13, d.points, 0)) as Q13, SUM(if(d.item_id = 19, d.points, 0)) as Q19, SUM(if(d.item_id = 25, d.points, 0)) as Q25, SUM(if(d.item_id = 31, d.points, 0)) as Q31, SUM(if(d.item_id = 37, d.points, 0)) as Q37, SUM(if(d.item_id = 43, d.points, 0)) as Q43, SUM(if(d.item_id = 49, d.points, 0)) as Q49, SUM(if(d.item_id = 55, d.points, 0)) as Q55, SUM(d.points) as total FROM tptd_response_details d INNER JOIN tptd_response r ON d.response_id = r.response_id INNER JOIN tptd_survey_items i ON d.item_id = i.item_id AND category = 'Purpose' WHERE r.order_line_item_id = :line_id GROUP BY d.response_id</sql></triaxia>
    <p>An intervening paragraph of content</p>
    <triaxia type=\"chart\" id=\"myGoogleChart2\"><data>...</data><background>...</background></triaxia>
    <p>A final paragraph of HTML content</p>";
    */
  
  $node = node_load(14);
  $string = $node->body['und'][0]['value'];
  
  //TODO: remove line breaks or improve preg_match pattern
  $output = $string; // preg_replace('/\n/', ' ', $string);
  $elements; // = array();
  // get all the triaxia xml nodes in the content
  $count = preg_match_all('/<triaxia.*?<\/triaxia>/i', $string, $elements);
  
  while($count > 0) {
    $count--;
    $elementstring = '<element>' . $elements[0][$count] . '</element>';
    $xml = simplexml_load_string($elementstring);
    $el = $xml->triaxia[0];
    $id = $el['id'];
    $type = $el['type'];
    // get the pattern to replace this unique node
    $pattern = '/<triaxia.*?id=[\'|"]' . $id . '[\'|"].*?>.*?<\\/triaxia>/i';
    
    // do some processing to create an HTML element from the xml node
    $new_element = '<div style="display:none"></div>';
    switch($type) {
      case 'datagrid':
        $new_element = render_datagrid($el);
        break;
      case 'chart':
        $new_element = render_chart($el);  // "<div id='$id' style='background-color:red'>$id will go here</div>"; // 
        break;
    }
    // and then replace in back in the content string
    $output = preg_replace($pattern, $new_element, $output);
  }
  //return $output;
  createPDF($output);
}

function render_datagrid($element) {
  $line_id = 1;
  $results = db_query($element->sql, array(':line_id' => $line_id));

  $header = preg_split('/,/', $element->headers); //array();
  $row_count = 0;
  $rows = array();
  foreach($results as $record) {
    $fields = get_object_vars($record);
    
    // get headers on first pass
    if($row_count == 0 && !$element->headers) {
      $header = array();
      foreach($fields as $delta => $field) {
        $header[] = $delta;
      }
      $row_count++;
    }
    
    // build row
    $row = array();
    foreach($fields as $delta => $field) {
        $row['data'][] = $field;    
    }
    $rows[] = $row;
  }
  
  $tablevars = array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => $element['id']), // 'purpose-table'),
    'caption' => NULL,
    'colgroups' => NULL,
    'sticky' => 0,
    'empty' => NULL,
  );
  
  return theme_table($tablevars);
}

function render_chart($element) {
  
  $line_item_id = 1; //TODO: Pass this into the function
  
  $chart_id = (string) $element['id'];
  $title = (string) $element->settings->title; // t('Team Survey Overview');
  $type = (string) $element->settings->type;
  $size_str = (string) $element->settings->size;
  $size = explode(',', $size_str);
  $data_colors = explode(',', (string) $element->settings->data_colors);
  $chart_fill = explode(',', (string) $element->settings->chart_fill);
  
  $chart = array(
    '#chart_id' => $chart_id,
    '#title' => chart_title($title, '3333ff', 22),
    '#type' => $type,
    '#size' => chart_size($size[0], $size[1]),
    '#data_colors' => $data_colors,
    '#chart_fill' => $chart_fill,
  );
  
  //$chart['#scale_range'] = array(0,5);
  $chart['#data_colors'][] = '8888ff';
  $chart['#chart_fill'] = array('bg', 's', '65432111');
  
  $sql = (string) $element->data->sql;
  $param_array = array(':line_item_id' => $line_item_id);
  
  $x_axis_type = (string) $element->data->x_axis['type'];
  $x_axis = (string) $element->data->x_axis;
  $y_axis_type = (string) $element->data->y_axis['type'];
  $y_axis = (string) $element->data->y_axis;
  $data_columns_str = (string) $element->data->data_columns;
  $data_columns = explode(',', $data_columns_str);
  $data_array = get_array_from_sql($sql, $param_array);
  
  // set data points
  foreach($data_array as $delta => $row) {
    foreach($data_columns as $column) {
      $chart['#data'][$delta] = $row[$column];
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
  case 'scale_range':
    //$chart['#mixed_axis_labels'][CHART_AXIS_X_BOTTOM][0][] = chart_mixed_axis_range_label($x_axis);
    $chart['#scale_range'] = explode(',', $x_axis);
    break;
 }
  
  $position = (string) $element->settings->position;
  $chart_url = chart_url($chart) . '&position=' . $position;
  
  $variables = array(
    'path' => $chart_url,
    'alt' => $title,
    'title' => $title,
    'attributes' => array('width' => $size[0], 'height' => $size[1]),
  );
  
  return theme_image($variables);
  
  
  //return '<div>Chart will go here</div>';
}
function render_chart_old($element) {
  
  $chart_url = get_diagnostic_chart_url();
  $variables = array(
    'path' => $chart_url,
    'alt' => 'The Chart',
    'title' => $element['id'],
    'attributes' => array('width' => 600, 'height' => 240),
  );
  
  return theme_image($variables);
  
  
  //return '<img width="65" height="73" alt="Home" src="http://www.google.com/intl/en_com/images/srpr/logo3w.png" />';
  // /themes/bartik/logo.png">';
  
}

function get_diagnostic_chart_url() {
  
  $chart = get_category_chart();
  
  $chart_url = chart_url($chart);
  return $chart_url . '&position=16x25';
}

/*
  $chart = array(
    '#chart_id' => 'test_chart',
    '#title' => chart_title(t('Team Scores'), '3333ff', 22),
    '#type' => CHART_TYPE_BAR_V, //CHART_TYPE_PIE_3D, //
    '#size' => chart_size(600, 400), //array('#width' => 840, '#height' => 480),
    '#grid_lines' => chart_grid_lines(30, 15),
    '#bar_size' => chart_bar_size(100, 25),
  );
  
  $query = db_select('tptd_survey_items', 'i');
  $query->join('tptd_response_details', 'd', 'i.item_id = d.item_id');
  $query->addExpression('SUM(d.points)', 'sum_points');
  $query->addField('i', 'category', 'category');
  $query->groupBy('i.category');
  
  $result = $query->execute();
  
  foreach($result as $row) {
    $chart['#data'][] = $row->sum_points;
    $chart['#labels'][] = t($row->category);
  }
  
  $chart['#data_colors'][] = '8888ff';
  $chart['#chart_fill'] = array('bg', 's', '65432111');
  */

function createPDF($report_content) {
    require_once(libraries_get_path('tcpdf') . '/config/lang/eng.php');
    require_once(libraries_get_path('tcpdf') . '/tcpdf.php');

    $pdf = new TCPDF(PDF_PAGE_ORIENTATION, PDF_UNIT, PDF_PAGE_FORMAT, true, 'UTF-8', false);

    // set document information
    $pdf->SetCreator(PDF_CREATOR);
    $pdf->SetAuthor('Marc Porlier');
    $pdf->SetTitle('TCPDF Example for Team Benchmark');
    $pdf->SetSubject('Team Benchmark Reporting');
    $pdf->SetKeywords('Team Benchmark, Diagnostics, Team');
    
    // set default header data
    $pdf->SetHeaderData(PDF_HEADER_LOGO, PDF_HEADER_LOGO_WIDTH, 'Triaxia Diagnostic Test', PDF_HEADER_STRING);
    
    // set header and footer fonts
    $pdf->setHeaderFont(Array(PDF_FONT_NAME_MAIN, '', PDF_FONT_SIZE_MAIN));
    $pdf->setFooterFont(Array(PDF_FONT_NAME_DATA, '', PDF_FONT_SIZE_DATA));
    
    // set default monospaced font
    $pdf->SetDefaultMonospacedFont(PDF_FONT_MONOSPACED);
    
    //set margins
    $pdf->SetMargins(PDF_MARGIN_LEFT, PDF_MARGIN_TOP, PDF_MARGIN_RIGHT);
    $pdf->SetHeaderMargin(PDF_MARGIN_HEADER);
    $pdf->SetFooterMargin(PDF_MARGIN_FOOTER);
    
    //set auto page breaks
    $pdf->SetAutoPageBreak(TRUE, PDF_MARGIN_BOTTOM);
    
    //set image scale factor
    $pdf->setImageScale(PDF_IMAGE_SCALE_RATIO);
    
    //set some language-dependent strings
    $pdf->setLanguageArray($l);
    
    // ---------------------------------------------------------
    
    // set default font subsetting mode
    $pdf->setFontSubsetting(true);
    
    // Set font
    // dejavusans is a UTF-8 Unicode font, if you only need to
    // print standard ASCII chars, you can use core fonts like
    // helvetica or times to reduce file size.
    $pdf->SetFont('dejavusans', '', 14, '', true);
    
    // Add a page
    // This method has several options, check the source code documentation for more information.
    $pdf->AddPage();
    // Set some content to print
    $html = $report_content;
    // Print text using writeHTMLCell()
    
    $pdf->writeHTMLCell($w=0, $h=0, $x='', $y='', $html, $border=0, $ln=1, $fill=0, $reseth=true, $align='', $autopadding=true);
    
    /*
    $chart_url = get_diagnostic_chart_url();
    if ($stream = fopen($chart_url, 'r')) {
        $url_image= stream_get_contents($stream, -1);
        fclose($stream);
    }
    
    $pdf->Image('@' . $url_image);
    */
    
    // ---------------------------------------------------------
    
    // Close and output PDF document
    // This method has several options, check the source code documentation for more information.
    $pdf->Output('example_001.pdf', 'I');
    exit();
}

/*
function get_google_chart_data_stream($chart_url) {
  
  $chart_base = 'http://chart.apis.google.com/chart?';
  $base_len = strlen($chart_base);
  $raw_query_str = substr($chart_url, $base_len);
  $encoded_query = str_replace('%3D', '=', rawurlencode($raw_query_str));
  $encoded_query = str_replace('%26', '&', $encoded_query);
  
  $chart_url = $chart_base . $encoded_query; //str_replace('%2F', '/', rawurlencode($chart_url));
  if ($stream = fopen($chart_url, 'r')) {
    $url_image= stream_get_contents($stream, -1);
    fclose($stream);
  }
    
  return '@' . $url_image;
}
*/

function get_test_content_1() {
  // this test mimics a node containing pure XML content
  $string = "<document>
    <p>Forty What?</p>
    <div style='float:right'>Joe</div>
    <p>Jane</p>
    <div>
     I know that's the answer -- but what's the question?
    </div>
    <triaxia type='datagrid'><sql>SELECT * FROM tptd_response_details</sql></triaxia>
    </document>";

  $xml = simplexml_load_string($string);
  $content = '<h3>' . $xml->triaxia[0]['type'] . '</h3>';
  $content .= '<p>' . $xml->triaxia[0]->sql . '</p>';
  //now replace the contents...
  $xml->triaxia[0] = '<table><tr><td>Hello Table</td></tr></table>';
  $newTable = $xml->triaxia[0];  // preg_replace('/<triaxia.*? >|</triaxia>/', '', $xml->asXML());
  $content .= '<p>' . $newTable . '</p>';
  
  return $content;
}

function build_data_table($nid) {
  $build = '<h2>Purpose</h2>';
  $build .= get_datagrid_results(1);
  $build .= '<br /><br /><span>Summary Data</span>';
  return $build;
}

function get_datagrid_results($line_id) {

  //TODO: these queries can be stored in the database as report items
  $sql = 'SELECT d.response_id as id, 
    SUM(if(d.item_id = 1, d.points, 0)) as Q1,
    SUM(if(d.item_id = 7, d.points, 0)) as Q7,
    SUM(if(d.item_id = 13, d.points, 0)) as Q13,
    SUM(if(d.item_id = 19, d.points, 0)) as Q19,
    SUM(if(d.item_id = 25, d.points, 0)) as Q25,
    SUM(if(d.item_id = 31, d.points, 0)) as Q31,
    SUM(if(d.item_id = 37, d.points, 0)) as Q37,
    SUM(if(d.item_id = 43, d.points, 0)) as Q43,
    SUM(if(d.item_id = 49, d.points, 0)) as Q49,
    SUM(if(d.item_id = 55, d.points, 0)) as Q55,
    SUM(d.points) as total
    FROM tptd_response_details d
    INNER JOIN tptd_response r ON d.response_id = r.response_id
    INNER JOIN tptd_survey_items i ON d.item_id = i.item_id AND category = \'Purpose\'
    WHERE r.order_line_item_id = :line_id
    GROUP BY d.response_id';
  
  $results = db_query($sql, array(':line_id' => $line_id));

  $header = array();
  $row_count = 0;
  $rows = array();
  foreach($results as $record) {
    $fields = get_object_vars($record);
    
    // get headers on first pass
    if($row_count == 0) {
      foreach($fields as $delta => $field) {
        $header[] = $delta;
      }
      $row_count++;
    }
    
    // build row
    $row = array();
    foreach($fields as $delta => $field) {
        $row['data'][] = $field;    
    }
    $rows[] = $row;
  }
  
  $tablevars = array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'purpose-table'),
    'caption' => NULL,
    'colgroups' => NULL,
    'sticky' => 0,
    'empty' => NULL,
  );
  
  return theme_table($tablevars);
}

/*
<p>Forty What?</p>
<div style='float:right'>Joe</div>
<p>Jane</p>
<triaxia id='myGoogleChart1' type='chart'><sql>SELECT * FROM tptd_response_details</sql></triaxia>
<div>I know that's the answer -- but what's the question?</div>
<triaxia id='myDataGrid' type='datagrid'><headers>Member,Q1,Q7,Q13,Q19,Q25,Q31,Q37,Q43,Q49,Q55,Total</headers><sql>SELECT r.team_member_email as email, SUM(if(d.item_id = 1, d.points, 0)) as Q1, SUM(if(d.item_id = 7, d.points, 0)) as Q7, SUM(if(d.item_id = 13, d.points, 0)) as Q13, SUM(if(d.item_id = 19, d.points, 0)) as Q19, SUM(if(d.item_id = 25, d.points, 0)) as Q25, SUM(if(d.item_id = 31, d.points, 0)) as Q31, SUM(if(d.item_id = 37, d.points, 0)) as Q37, SUM(if(d.item_id = 43, d.points, 0)) as Q43, SUM(if(d.item_id = 49, d.points, 0)) as Q49, SUM(if(d.item_id = 55, d.points, 0)) as Q55, SUM(d.points) as total FROM tptd_response_details d INNER JOIN tptd_response r ON d.response_id = r.response_id INNER JOIN tptd_survey_items i ON d.item_id = i.item_id AND category = 'Purpose' WHERE r.order_line_item_id = :line_id GROUP BY d.response_id</sql></triaxia>
<p>An intervening paragraph of content</p>
<triaxia type="chart" id="myGoogleChart2"><data>...</data><background>...</background></triaxia>
<p>A final paragraph of HTML content</p>
*/

  //$directory = file_create_url('public://report_templates');
  //$mypath = realpath($directory . '/' . 'team_conflict_template.pdf');
  //$mypath = dirname(__FILE__);
  //$mypath = drupal_realpath('public://report_templates');
  //return l('Test', $mypath, array());
  
  //$template_folder = file_create_filename('template-test.pdf', 'public://assets');
  //$template_folder = file_create_url('public://report_templates') . '/team_conflict_template.pdf';
  //return l('Test PDF File', $template_folder, array());
  
  //create_pdf();
  //return 'This is test output';
  //get_category_chart();
  
  //$content = get_status_pie(); // get_report_nodes(); //get_test_content_2(); // get_category_chart(); // 
  
  //$content = '<img id="chart-test_chart" class="chart" typeof="foaf:Image" src="http://chart.apis.google.com/chart?chxl=1:|Excellent+Communication|Solid+Relationships|Effective+Processes|Accepted+Leadership|Clear+Roles|Common+Purpose&chxt=x,y&chbh=a&chs=600x240&cht=bhs&chd=s:fSlMrx&chtt=Team+Survey+Overview" alt="Team Survey Overview 3333ff 22" />';
  /*
  $build = array(
    'rendered_output' => array(
      '#type' => 'markup',
      '#markup' => "<p>$content</p>",
    ),
  );
  */
