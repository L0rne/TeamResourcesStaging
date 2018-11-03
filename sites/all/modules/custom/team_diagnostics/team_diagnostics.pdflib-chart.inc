<?php
/**
 *  Generates pCharts for PDFLib
 *
 */
require_once(libraries_get_path('pchart') . '/class/pData.class.php');
require_once(libraries_get_path('pchart') . '/class/pDraw.class.php');
require_once(libraries_get_path('pchart') . '/class/pImage.class.php');
require_once(libraries_get_path('pchart') . '/class/pPie.class.php');
require_once(libraries_get_path('pchart') . '/class/pRadar.class.php');
require_once(libraries_get_path('pchart') . '/class/pScatter.class.php');

require_once(dirname(__FILE__) . '/team_diagnostics.css-parser.inc');
require_once(dirname(__FILE__) . '/team_diagnostics.data-manipulation.inc');

//define('PCHART_FONT_DIRECTORY', libraries_get_path('pchart') . '/fonts/');
//define('PCHART_PALETTE_DIRECTORY', libraries_get_path('pchart') . '/palettes/');
//define('PCHART_RESOURCE_DIRECTORY', libraries_get_path('pchart') . '/resources/');
//define('PDFLIB_CHART_DIRECTORY',

/**
 *  This factory is an interface between the DiagnosticDMO and mlpCharts. Ideally,
 *  DiagnosticDMO should be agnostic to mlpCharts and vice versa.
 *
*/
class PdfLibChartFactory {
    
    public static function get_single_bar($chart_args, $datum) {
        $data = array(
            'series'    => $datum,
            'abscissa'  => 'Mean',
            'axes'      => array(),
        );
        
        return new mlpBarChart($chart_args, $data);
    }
    
    public static function create_chart($chart_args = array('data_key' => 'category_means'), &$dmo) {
        
        $debug_vars = array('file' => __FILE__, 'fn' => __FUNCTION__);
        
        //$data = self::get_data($chart_args['data_key'], $dmo);
        $data = array(
            'series'    => array(),
            'abscissa'  => array(),
            'axes'      => array(),
        );
        
        //die(var_dump($chart_args));
        $data['series'] = $dmo->get_data_series($chart_args);
        //die(var_dump($data['series']));
        $data['abscissa']['labels'] = array_keys(current($data['series'])); //$dmo->get_categories(); //TODO: assign this to a more responsible area of the code
        $data['abscissa']['description'] = 'Categories';

        $chart_type = $chart_args['chart_type'];
        $chart;
        
        switch ($chart_type) {
            case 'bar':
            case 'barchart':
                self::format_data($data);
                
                if (!isset($chart_args['class'])) $chart_args['class'] = 'barchart';
                if (isset($chart_args['axis_max'])) {
                    $axis_boundaries[] = array(
                        'Min' => (float)$chart_args['axis_min'],
                        'Max' => (float)$chart_args['axis_max'],
                    );
                    $chart_args['axis_boundaries'] = $axis_boundaries;
                }
                
                $chart = new mlpBarChart($chart_args, $data);
                break;
            case 'scatterplot':
                
                if (!isset($chart_args['class'])) $chart_args['class'] = 'scatterplot';
                
                //TODO: Fix this! You need one axis_boundary array for x and one for y. No more,
                //      regardles of how many series are in the dataset.
                $axis_boundaries = array();
                //foreach ($data['series'] as $serie) {
                for ($inc = 0; $inc < 2; $inc++) {
                    $axis_boundaries[] = array(
                        'Min' => (float)$chart_args['axis_min'],
                        'Max' => (float)$chart_args['axis_max'],
                    );
                }
                
                $chart_args['axis_boundaries'] = $axis_boundaries;
                
                $chart = new mlpScatterPlot($chart_args, $data);
        
                break;
            case 'radar':
                //$debug_vars['data_series'] = $data['series'];
                //$debug_vars['chart'] = $chart_args;
                //die(var_dump($debug_vars));
                
                self::format_data($data);
                if (!isset($chart_args['class'])) $chart_args['class'] = 'radar-chart';
                $chart = new mlpRadarChart($chart_args, $data);
                break;
        }
        
        return $chart;
    }
    
    protected static function format_data(&$data) {
        foreach ($data['series'] as $series_name => $series) {
            foreach ($series as $key => $point) {
                $data['series'][$series_name][$key] = number_format($point, 2);
            }
        }
    }
    
    //protected static function get_data($data_key, $dmo) {
    //    return $dmo;
    //}
}

/**
 *  These classes are mostly agnostic to PDFLib, the Team Diagnostic data, and the business
 *  requirements for Team Benchmark. PdfLibChartFactory uses the DiagnosticDMO to shape the
 *  data array for each of the chart subclasses. Ideally, the classes below could be ported
 *  to a completely new project, requiring only a new factory class to shape the data.
 *
*/
abstract class mlpChart {
    
    protected $data;
    protected $canvas;
    protected $properties = array();
    protected $id;
    protected $graph_area;
    protected static $counter = 0;
    protected $image_folder;
    protected $styles; // a css-parser object
    protected $image_filename;
    protected $scatter_serie_image;
    public $image_location;
    
    function __construct($chart_args, $data = array()) {
        
        $this->id = $chart_args['id']; // ?
        unset($chart_args['id']);
        
        $this->styles = $chart_args['css'];
        unset($chart_args['css']);
        
        $this->image_folder = variable_get('pchart_api_image_directory');
        
        //die(var_dump($this->image_folder));
        //isset($chart_args['image_folder']) ? file_create_url($chart_args['image_folder']) :
        if (isset($chart_args['image_folder_uri'])) {
            $url = file_create_url($chart_args['image_folder_uri']); // URI to URL
            $parsed_url = parse_url($url);                       // URL to parsed array
            $path = substr($parsed_url['path'], 1) . '/';        // relative path
            $this->image_folder = $path;
            //die(var_dump($path));
        }
        
        if (isset($chart_args['scatter_serie_image'])) {
            $this->scatter_serie_image = $this->image_folder . $chart_args['scatter_serie_image'];
        }
        
        $this->image_filename = $this->id . '_' . self::$counter++ . '.png';
        
        $this->properties = $chart_args;
        $this->data = new pData();
        
        // call chart method sequence
        $this->prepare_dataset($data);
        $this->initialize_canvas();
        $this->define_graph_area();
        $this->set_font();
        $this->compute_scale();
        $this->draw_chart();
        $this->render_chart();
    }
    
    function __destruct() {}
    
    //function get_chart_url() {}
    
    // 1. Prepare your dataset*
    abstract protected function prepare_dataset($data = array());
    
    // 2. Create your pChart Object (initialize the drawing canvas)
    protected function initialize_canvas($args = array()) {
        $this->canvas = new pImage($this->properties['width'], $this->properties['height'], $this->data, TRUE);
        
        if (isset($this->properties['background_image'])) {
            
            $bg_image = $this->properties['background_image'];
            $last_dot = strrpos($bg_image, '.');
            $img_ext = strtolower(substr($bg_image, $last_dot + 1));
            
            $bg_path_file = $this->image_folder . $bg_image;
            switch ($img_ext) {
                case 'png':
                    $this->canvas->drawFromPNG(0, 0, $bg_path_file);
                    break;
                case 'gif':
                    $this->canvas->drawFromGIF(0, 0, $bg_path_file);
                    break;
                case 'jpg':
                case 'jpeg':
                    $this->canvas->drawFromJPG(0, 0, $bg_path_file);
                    break;
            }
        }
    }
    
    // 3. Define your chart area (graph area)
    protected function define_graph_area($args = array()) {
        $padding_str = $this->get_style('.graph-area', 'padding', '3 3 3 3'); //array(7, 22, 25, 17); // use CSS padding syntax to set the Chart Area
        $chart_area_padding = explode(' ', trim($padding_str));
        if (count($chart_area_padding) != 4) $chart_area_padding = array(3,3,3,3);
        
        $top_left_x = $chart_area_padding[3];
        $top_left_y = $chart_area_padding[0];
        $bottom_right_y = $this->properties['height'] - $chart_area_padding[2];
        $bottom_right_x = $this->properties['width'] - $chart_area_padding[1];
        $this->canvas->setGraphArea($top_left_x, $top_left_y, $bottom_right_x, $bottom_right_y);
        
    }
    
    // 4. Choose a nice font
    protected function set_font() {
        
        //$css_font = $this->get_style('.canvas', 'font-family', 'cartogothicpro-book');
        $font_name = $this->image_folder . 'cartogothicpro-black.otf'; // 'miso-webfont.ttf'; //TODO: implement from above CSS call using "search for TTF file" logic
        
        //die($font_name);
        
        $font_size = $this->get_style('.canvas', 'font-size', 10);
        $font_color = $this->hex_color_to_rgb_array($this->get_style('.canvas', 'color', '#000000'));
        $font_alpha = $this->get_style('.canvas', 'opacity', 1) * 100;
        $font_alpha = $font_alpha > 100 ? 100 : $font_alpha;
        $font_props = array(
          'FontName' => $font_name,
          'FontSize' => $font_size,
          "R" => $font_color['R'], "G" => $font_color['G'], "B" => $font_color['B'], 
          "Alpha" => $font_alpha, // this setting has no effect on scale numbers even though the colors do!
        );
        $this->canvas->setFontProperties($font_props);
    }
    
    // 5. Compute the scale* (not all subclasses will implement this method, so we just call an empty method in the base class.)
    protected function compute_scale() {}
    
    // 6. Draw your chart*
    abstract protected function draw_chart(); // {}
    
    // 7. (various) Ouput
    protected function render_chart() {
        $this->image_location = $this->image_folder . $this->image_filename;
        $this->canvas->render($this->image_location);
    }
    
    public function get_html() {
        $chart = array(
          'path' => $this->image_location,
          'alt' => 'Chart Image', //TODO: set from properties
          'title' => 'pChart for Team Diagnostics', //TODO: set from properties
          'attributes' => array(), //TODO: set from properties
        );
        return theme_image($chart);
    }
    
    public function get_absolute_path() {
        return $image_folder = drupal_realpath($this->properties['image_folder_uri']) . '/' . $this->image_filename;
    }
    
    // helper methods
    protected function get_style($selector, $property, $default = false) {
        $css_id = '#' . preg_replace('/_/', '-', $this->id) . ' ' . $selector;
        $base_class = '.' . $this->properties['base_class'] . '-chart ' . $selector;
        $block_class = '.' . $this->properties['class'] . ' ' . $selector;
        $value = $default;
        
        //$styles = new triaxia_CSSParser();
        if ($value = $this->styles->Get($css_id, $property)) return $value;
        if ($value = $this->styles->Get($block_class, $property)) return $value;
        if ($value = $this->styles->Get($base_class, $property)) return $value;
        if ($value = $this->styles->Get($selector, $property)) return $value;
        
        return $value ? $value : $default;
    }
    
    protected function hex_color_to_rgb_array($color) {
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
    
    
        return array('R' => hexdec($r), 'G' => hexdec($g), 'B' => hexdec($b));
    }
    
}

class mlpBarChart extends mlpChart {
    private $palette = array();
    
    protected function prepare_dataset($data = array()) {
        //$this->data->addPoints(array(3.88,4.05,3.83,3.98,2.00,4.02), 'Means');
        // add series
       
        foreach ($data['series'] as $series_name => $series) {
            $this->data->addPoints($series, $series_name);
            
            foreach ($series as $data_value) {
                
                if ($data_value > 4.0)
                    $this->palette[] = array("R"=>0,"G"=>255,"B"=>0,"Alpha"=>100);
                
                if ($data_value > 3.5 && $data_value <= 4.0)
                    $this->palette[] = array("R"=>255,"G"=>255,"B"=>0,"Alpha"=>100);
                
                if ($data_value <= 3.5)
                    $this->palette[] = array("R"=>255,"G"=>0,"B"=>0,"Alpha"=>100);
            }
        }
        
        // set axis labels
        ////$data->setAxisName(0, 'Team Average'); Set this if you want to display the Axis name on the chart
        //$this->data->addPoints(array('Common Purpose','Clear Roles','Accepted Leadership','Effective Processes','Solid Relationships','Excellent Communication'), 'Categories');
        //$this->data->setSerieDescription('Categories', 'Survey Categories');
        //$this->data->setAbscissa('Categories');
        $this->data->addPoints($data['abscissa']['labels'], 'labels');
        $this->data->setSerieDescription('labels', $data['abscissa']['description']);
        $this->data->setAbscissa('labels');
        
        // set data colors
        //$this->data->loadPalette(PCHART_PALETTE_DIRECTORY . 'evening.color', TRUE);
        
          // set axis position
        $axis_position = strtoupper($this->properties['bar_position']) == 'HORIZONTAL' ? AXIS_POSITION_RIGHT : AXIS_POSITION_LEFT;
        $this->data->setAxisPosition(0, $axis_position);  // if bar chart uses horizontal bars, set Axis to the "Right"
                                                    // Confused? You should be. To render horizontal bars, pChart
                                                    // turns the entire graph area clockwise 90¡ so the Y-Axis is
                                                    // on the top and the X-Axis is on the left. 
    }
    
    protected function compute_scale() {
        //$axis_boundaries = array(
        //    0 => array('Min' => 0,
        //               'Max' => 5),
        //);
        
        $scale_settings = array(
          'Factors'     => array(1), // scale units. display integers 1 - 5.
          'Pos'         => strtoupper($this->properties['bar_position']) == 'HORIZONTAL' ? SCALE_POS_TOPBOTTOM : SCALE_POS_LEFTRIGHT,
          'AxisAlpha'   => 0, // Set to zero if you don't want to see the axis lines
          'TickAlpha'   => 0, // "
          'GridAlpha'   => 0, // "
          'RemoveXAxis' => FALSE, // Set to TRUE if you don't want pChart to render the abscissa labels.
        );
        
        if (isset($this->properties['axis_boundaries'])) {
            $scale_settings['Mode'] = SCALE_MODE_MANUAL;
            $scale_settings['ManualScale'] = $this->properties['axis_boundaries'];
        }
        else {
            $scale_settings['Mode'] = SCALE_MODE_FLOATING;
        }
        
        $this->canvas->drawScale($scale_settings);
    }
    
    protected function draw_chart() {
        $bar_alpha = ($this->get_style('.bar', 'opacity', 1));
        
        
        /*
         * Old CSS defined palette logic...
         * 
        $bar_n = 1;
        $bar_n_selector = ".bar:nth-child(" . $bar_n . ")";
        
        
        $palette = array();
        
        while($bar_n_color = $this->get_style($bar_n_selector, 'color')) {
            $bar_color = $this->hex_color_to_rgb_array($bar_n_color);
        
            $palette[$bar_n-1] = $bar_color;
            $alpha = $this->get_style($bar_n_selector, 'opacity', $bar_alpha);
            $palette[$bar_n-1]["Alpha"] = $alpha * 100;
            $bar_n_selector = ".bar:nth-child(" . ++$bar_n . ")";
        
        }
        **********************
        **********************
        just for testing...
        **********************
        $alpha = 80;
        $palette = array(
            "0"=>array("R"=>0,"G"=>0,"B"=>0,"Alpha"=>$alpha),
            "1"=>array("R"=>0,"G"=>0,"B"=>255,"Alpha"=>$alpha),
            "2"=>array("R"=>0,"G"=>255,"B"=>0,"Alpha"=>$alpha),
            "3"=>array("R"=>0,"G"=>255,"B"=>255,"Alpha"=>$alpha),
            "4"=>array("R"=>255,"G"=>0,"B"=>0,"Alpha"=>$alpha),
            "5"=>array("R"=>255,"G"=>0,"B"=>255,"Alpha"=>$alpha),
            "6"=>array("R"=>255,"G"=>255,"B"=>0,"Alpha"=>$alpha),
            "7"=>array("R"=>0,"G"=>0,"B"=>125,"Alpha"=>$alpha),
            "8"=>array("R"=>0,"G"=>125,"B"=>0,"Alpha"=>$alpha),
            "9"=>array("R"=>0,"G"=>125,"B"=>125,"Alpha"=>$alpha),
            "10"=>array("R"=>125,"G"=>0,"B"=>0,"Alpha"=>$alpha),
        );
        */
        
        
        $bar_chart_props = array(
          'DisplayValues' => TRUE,
          'DisplayPos'    => LABEL_POS_OUTSIDE, //LABEL_POS_INSIDE,
          'Rounded'       => TRUE,    // rounded bar corners
          'Interleave'    => 0.1,     // the space between the bars is equal to 1/10 the width of the bar
          //'OverrideColors' => $palette,
          'Decimals' => 1,
        );
        
        if (!empty($this->palette)) $bar_chart_props['OverrideColors'] = $this->palette;
        
        $this->canvas->drawBarChart($bar_chart_props);
    }
}

class mlpScatterPlot extends mlpChart {
    private $scatter;
    
    protected function prepare_dataset($data = array()) {
        $x_series = $this->properties['x_series'];
        $y_series = $this->properties['y_series'];
        
        $serie_index = 0;
        foreach ($data['series'] as $serie_key => $series) {
            // x-series of series pairing
            $this->data->addPoints($series[$x_series], $x_series);
            $this->data->setAxisName(0, 'X-Axis');
            $this->data->setAxisXY(0, AXIS_X);
            $this->data->setAxisPosition(0, AXIS_POSITION_BOTTOM);
            
            // Y-Axis
            $this->data->addPoints($series[$y_series], $y_series);
            $this->data->setSerieOnAxis($y_series, 1);
            $this->data->setAxisName(1, 'Y-Axis');
            $this->data->setAxisXY(1, AXIS_Y);
            $this->data->setAxisPosition(1, AXIS_POSITION_LEFT);
            
            // Bind Data Series into Plot
            $this->data->setScatterSerie($x_series, $y_series, $serie_index); // bind x & y axes
            $this->data->setScatterSerieDescription($serie_index, $serie_key);
            
            $serie_image = isset($this->scatter_serie_image ) ?
                $this->scatter_serie_image :
                $this->image_folder . 'serie' . $serie_index . '.png';
            $this->data->setScatterSeriePicture($serie_index, $serie_image);
            
            //$this->data->setScatterSerieColor($serie_index,array("R"=>147,"G"=>167,"B"=>115));
            //$this->data->setScatterSerieShape($serie_index, SERIE_SHAPE_FILLEDCIRCLE);
            
  
            $serie_index++;
        }
        
        //die(var_dump($this->data->Data));
    }
    
    protected function compute_scale() {
        //if ($this->properties['name'] != 'profile_skill_will_scatter')
        //    die(var_dump($this->properties));
        
        $test_rgb = 255; //255; //$this->properties['name'] == 'profile_skill_will_scatter' ? 255 : 100;

        $axis_boundaries = array(
            0 => array('Min' => isset($this->properties['axis_min']) ? $this->properties['axis_min'] : 0,
                       'Max' => isset($this->properties['axis_max']) ? $this->properties['axis_max'] : 150),
            1 => array('Min' => isset($this->properties['axis_min']) ? $this->properties['axis_min'] : 0,
                       'Max' => isset($this->properties['axis_max']) ? $this->properties['axis_max'] : 150),
        );
        $scale_settings = array(
          'Mode'        => SCALE_MODE_MANUAL,
          'ManualScale' => $axis_boundaries, // $this->properties['axis_boundaries'],
          'Factors'     => array(10), // scale units. display integers 1 - 5.
          'GridR'       => $test_rgb, // set to 100 for testing; 255 for production
          'GridG'       => $test_rgb, // "
          'GridB'       => $test_rgb, // "
          'GridAlpha'   => 0, // set to 100 for testing; 0 for production
          'AxisR'       => 255,
          'AxisG'       => 255,
          'AxisB'       => 255,
          'AxisAlpha'   => 0, // don't show it for now!
          'TickR'       => 255,
          'TickR'       => 255,
          'TickR'       => 255,
          'TickAlpha'   => 0,
          'DrawXlines'  => FALSE, //change to TRUE for testing
          'DrawYLines'  => ALL,
        );
        
        $this->scatter = new pScatter($this->canvas, $this->data);
        $this->scatter->drawScatterScale($scale_settings);
        
    }
    
    protected function draw_chart() {
        $this->scatter->drawScatterPlotChart();
    }
}

class mlpRadarChart extends mlpChart {
    private $radar;

    protected function prepare_dataset($data = array()) {
        $debug_vars = array('file' => __FILE__, 'class' => __CLASS__, 'fn' => __FUNCTION__, 'line' => __LINE__);
        $debug_vars['abscissa'] = $data['abscissa'];
        $debug_vars['properties'] = $this->properties;
        //die(var_dump($debug_vars));
        
        $serie_index = 0;
        foreach ($data['series'] as $serie_key => $series) {
            // x-series of series pairing
            $this->data->addPoints($series, $serie_key);
            
            $serie_name = preg_replace('/_/', ' ', $serie_key);
            $serie_name = ucwords($serie_name);
            $this->data->setSerieDescription($serie_key, $serie_name);
            
            $series_settings = array("R"=>0,"G"=>0,"B"=>0,"BorderR"=>0,"BorderG"=>0,"BorderB"=>0,"Alpha"=>100);
            $this->data->setPalette($serie_key, $series_settings);
            //$this->data->setSerieShape($serie_key, SERIE_SHAPE_FILLEDTRIANGLE);
            $serie_index++;
        }
        
        //$this->data->addPoints($data['abscissa']['labels'], 'labels');
        //$this->data->setSerieDescription('labels', $data['abscissa']['description']);
        //$this->data->setAbscissa('labels');
        
        //die(var_dump($this->data->Data));
    }
    
    protected function compute_scale() { /* No scale computation is necessary for radar pCharts */ }
    
    protected function draw_chart() {
        $this->radar = new pRadar();
        $props = $this->get_radar_properties();
        
        $options = array(
                  "AxisR" => $props['axis']['color']['R'],
                  "AxisG" => $props['axis']['color']['G'],
                  "AxisB" => $props['axis']['color']['B'],
                  "AxisAlpha" => $props['axis']['alpha'], // set to 100 for testing
                  "AxisFontSize" => $props['axis']['font-size'], // set to 12 for testing
                  "LabelsBGR" => $props['labels']['backgound-color']['R'],
                  "LabelsBGG" => $props['labels']['backgound-color']['G'],
                  "LabelsBGB" => $props['labels']['backgound-color']['B'],
                  "LabelsBGAlpha" => $props['labels']['alpha'], // set to 100 for testing
                  "AxisRotation" => $props['axis']['rotation'],
                  "SegmentHeight" => 1,
                  "Segments" => 5,
                  "DrawLines" => TRUE,
                  "DrawPoly"=>TRUE,
                  "PolyAlpha" => 40,
                  "DrawPoints" => TRUE,
                  "PointRadius" => 2,
                  "PointSurrounding" => 10,
                  "OuterBubbleRadius" => $props['outer-bubble']['radius'],
                  "OuterBubbleR" => $props['outer-bubble']['background-color']['R'],
                  "OuterBubbleG" => $props['outer-bubble']['background-color']['G'],
                  "OuterBubbleB" => $props['outer-bubble']['background-color']['B'],
                  "OuterBubbleAlpha" => $props['outer-bubble']['alpha'],
                  "InnerBubbleR" => $props['inner-bubble']['background-color']['R'],
                  "InnerBubbleG" => $props['inner-bubble']['background-color']['G'],
                  "InnerBubbleB" => $props['inner-bubble']['background-color']['B'],
                  "InnerBubbleAlpha" => $props['inner-bubble']['alpha'],
                  "WriteValues"=> TRUE,
                  //"ValueFontName" => 'arial', //'cartogothicpro-black.otf', //?
                  "ValueFontSize"=> $props['values']['font-size'], // set to 16 for testing
                  "ValuePadding" => $props['values']['padding'],
                  "WriteValuesInBubble" => TRUE,
                  "LabelPos" => RADAR_LABELS_ROTATED, //  RADAR_LABELS_HORIZONTAL, // 
                  "Layout" => $props['axis']['shape'], //RADAR_LAYOUT_CIRCLE, // RADAR_LAYOUT_STAR, // 
                  "BackgroundGradient" => array(
                      "StartR"=>255,"StartG"=>255,"StartB"=>255,"StartAlpha"=>0,
                      "EndR"=>255,"EndG"=>255,"EndB"=>255,"EndAlpha"=>0,
                  ),
              );
        //die(var_dump($options));
        $this->radar->drawRadar($this->canvas, $this->data, $options);
    }
    
    private function get_radar_properties() {
        $radar_props = array();
        
        $radar_props['axis']['rotation'] = (float)$this->get_style('.axis', 'rotation', 1);
        
        $radar_props['axis']['font-size'] = $this->get_style('.axis', 'font-size', 10);
        $axis_color = $this->get_style('.axis', 'color', '#000000');
        $radar_props['axis']['color'] = $this->hex_color_to_rgb_array($axis_color);
        $alpha = $this->get_style('.axis', 'opacity', 1);
        $radar_props['axis']['alpha'] = $alpha * 100;
        $shape = $this->get_style('.axis', 'background-image', 'star');
        $radar_props['axis']['shape'] = stristr($shape, 'star') ? RADAR_LAYOUT_STAR : RADAR_LAYOUT_CIRCLE;
        
        $label_color = $this->get_style('.labels', 'background-color', '#ffffff');
        $radar_props['labels']['backgound-color'] = $this->hex_color_to_rgb_array($label_color);
        $alpha = $this->get_style('.labels', 'opacity', 1);
        $radar_props['labels']['alpha'] = $alpha * 100;
        
        $color = $this->get_style('.outer-bubble', 'background-color', '#ffffff');
        $radar_props['outer-bubble']['background-color'] = $this->hex_color_to_rgb_array($color);
        $radar_props['outer-bubble']['radius'] = $this->get_style('.outer-bubble', 'size', 6);
        $alpha = $this->get_style('.outer-bubble', 'opacity', 1);
        $radar_props['outer-bubble']['alpha'] = $alpha * 100;
        
        $color = $this->get_style('.inner-bubble', 'background-color', '#333333');
        $radar_props['inner-bubble']['background-color'] = $this->hex_color_to_rgb_array($color);
        $radar_props['inner-bubble']['radius'] = $this->get_style('.inner-bubble', 'size', 6);
        $alpha = $this->get_style('.inner-bubble', 'opacity', 1);
        $radar_props['inner-bubble']['alpha'] = $alpha * 100;
        
        $radar_props['values']['font-size'] = $this->get_style('.values', 'font-size', 8);
        $radar_props['values']['padding'] = $this->get_style('.values', 'padding', 8);
        
        return $radar_props;
    }
}
