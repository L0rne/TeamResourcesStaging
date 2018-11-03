<?php
// $Id$

/**
 * @file
 * A Drupalesque wrapper class for pChart
 *
 * Chart basics:
 http://wiki.pchart.net/doc.basic.syntax.html
 *
 * For data functions: 
 * 
 * For specific charts, see:
 * http://wiki.pchart.net/doc.chart.drawbarchart.html
 * http://wiki.pchart.net/doc.chart.drawplotchart.html
 * http://wiki.pchart.net/doc.scatter.drawscatterplotChart.html
 * http://wiki.pchart.net/doc.pie.basics.html
 * http://wiki.pchart.net/doc.draw.radar.html
 * 
 */

require_once(libraries_get_path('pchart') . '/class/pData.class.php');
require_once(libraries_get_path('pchart') . '/class/pDraw.class.php');
require_once(libraries_get_path('pchart') . '/class/pImage.class.php');
require_once(libraries_get_path('pchart') . '/class/pPie.class.php');
require_once(libraries_get_path('pchart') . '/class/pRadar.class.php');
require_once(libraries_get_path('pchart') . '/class/pScatter.class.php');


define('PCHART_FONT_DIRECTORY', libraries_get_path('pchart') . '/fonts/');
define('PCHART_PALETTE_DIRECTORY', libraries_get_path('pchart') . '/palettes/');
define('PCHART_RESOURCE_DIRECTORY', libraries_get_path('pchart') . '/resources/');

class DrupalpChartFactory {
    //static $default_settings = array(
    //    "Pos"=>SCALE_POS_LEFTRIGHT,
    //    "Mode"=>SCALE_MODE_START0,
    //    "LabelingMethod"=>LABELING_ALL,
    //    "GridR"=>255, "GridG"=>255, "GridB"=>255, "GridAlpha"=>50,
    //    "TickR"=>0, "TickG"=>0, "TickB"=>0, "TickAlpha"=>50,
    //    "LabelRotation"=>0,
    //    "CycleBackground"=>1,
    //    "DrawXLines"=>0,
    //    "DrawYLines"=>NONE
    //);
    
    static public function create_chart($chart_args) {
        $type = $chart_args['chart']['type']['type'];
        switch ($type) {
            case 'bar':
                $bar_chart = new DrupalpBarChart($chart_args);
                return $bar_chart;
                break;
            case 'scatterplot':
                return new DrupalpScatterPlotChart($chart_args);
                break;
            case 'radar':
                return new DrupalpRadarChart($chart_args);
                break;
        }
    }
}

abstract class DrupalpChart {
    protected $dpData; // = new pData();
    protected $dpImage;
    protected $id;
    protected $graph_area;
    protected static $counter = 0;
    static public $image_folder; 
    public $chart_args;
    
    function __construct($args) { // $args = array('id' => 'default', 'width' => 400, 'height' => 400, 'data' => null)) {
        $this->dpData = new pData();
        $this->id = $args['id'] . self::$counter++;
        $this->chart_args = $args;
        $this->calculateGraphArea($args);
        self::$image_folder = variable_get('pchart_api_image_directory');
    }
    
    abstract protected function set_data_points($args);
    abstract protected function drawScale($args);
    abstract protected function drawChart($args);
    
    protected function calculateGraphArea($args) {
        $this->graph_area['x'] = isset($args['chart']['graph']['left-margin']) ? $args['chart']['graph']['left-margin'] : 100;
        $this->graph_area['y'] = isset($args['chart']['graph']['top-margin']) ? $args['chart']['graph']['top-margin'] : 1;
        $this->graph_area['width'] = isset($args['chart']['graph']['right-margin']) ?
            $args['settings']['size']['width'] - $args['chart']['graph']['right-margin'] :
            $args['settings']['size']['width'] - 1;
        $this->graph_area['height'] = isset($args['chart']['graph']['bottom-margin']) ?
            $args['settings']['size']['height'] - $args['chart']['graph']['bottom-margin'] :
            $args['settings']['size']['height'] - 1;
    }
    
    public function render() {
        $image_location = self::$image_folder . $this->id . '.png';
        $this->dpImage->render($image_location);
        return $image_location;
    }
}

class DrupalpRadarChart extends DrupalpChart {
    function __construct($args) {
        parent::__construct($args);
        $this->set_data_points($args);
        $this->drawChart($args);
    }
    
    protected function set_data_points($args) {
        // Create and populate the pData object
        
        $series = array_values($args['data']['series']);
        $names = array_keys($args['data']['series']);
        $this->dpData->addPoints($series[0], $names[0]);
        
        $series_settings = $args['settings']['color_override'][0]; // array("R"=>229,"G"=>11,"B"=>11,"Alpha"=>100);
        //$series_settings = array("R"=>191,"G"=>15,"B"=>59,"BorderR"=>211,"BorderG"=>235,"BorderB"=>79,"Alpha"=>80);
        $this->dpData->setPalette($names[0], $series_settings);
        
        // set abscissa
        $this->dpData->addPoints($args['data']['abscissa']['values'], $args['data']['abscissa']['name']);
        $this->dpData->setAbscissa($args['data']['abscissa']['name']);
    }
    
    protected function drawScale($args) {}
    
    protected function drawChart($args) {
        /* Create the pChart object */
        $this->dpImage = new pImage($args['settings']['size']['width'], $args['settings']['size']['height'], $this->dpData, TRUE);
        
        /* Set the default font */
        $font_name = 'calibri.ttf'; // $args['settings']['font-name'] . '.ttf';
        $font_size = $args['settings']['font-size'];
        $this->dpImage->setFontProperties(array(
            "FontName" => PCHART_FONT_DIRECTORY . $font_name,
            "FontSize" => $font_size,
            "R" => 0, "G" => 0, "B" => 0, 
            "Alpha" => 100,
            )
        );
        
        /* Enable shadow computing */ 
        //$this->dpImage->setShadow(TRUE,array("X"=>2,"Y"=>2,"R"=>20,"G"=>20,"B"=>20,"Alpha"=>30));
        
        /* Create the pRadar object */ 
        $radarChart = new pRadar();
        
        /* Draw a radar chart */
        $max_boundary = (float) $args['scale']['axis_boundaries']['Max'];
        $this->dpImage->setGraphArea($this->graph_area['x'], $this->graph_area['y'], $this->graph_area['width'], $this->graph_area['height']);
        $options = array(
            //"AxisR" => 100, "AxisG" => 40, "AxisB" => 220, "AxisAlpha" => 100,
            //"LabelsBGR" => 40, "LabelsBGG" => 40, "LabelsBGB" => 40, "LabelsBGAlpha" => 70,
            "AxisRotation" => 30,
            "SegmentHeight" => 1,
            "Segments" => $max_boundary,
            "DrawPoly"=>TRUE,
            "OuterBubbleRadius" => 10,
            "OuterBubbleR" => 0, "OuterBubbleG" => 46, "OuterBubbleB" => 184, "OuterBubbleAlpha" => 80,
            //"InnerBubbleR" => 20, "InnerBubbleG" => 20, "InnerBubbleB" => 20, "InnerBubbleAlpha" => 20,
            "PolyAlpha" => 20,
            "WriteValues"=>TRUE,
            "ValueFontSize"=> $font_size,
            "ValuePadding" => 10,
            "WriteValuesInBubble" => TRUE,
            "LabelPos" => RADAR_LABELS_HORIZONTAL, // RADAR_LABELS_ROTATED, //  
            "Layout" => RADAR_LAYOUT_STAR, // RADAR_LAYOUT_CIRCLE, //
            //"BackgroundGradient" => array(
            //    "StartR"=>255,"StartG"=>255,"StartB"=>255,"StartAlpha"=>100,
            //    "EndR"=>207,"EndG"=>227,"EndB"=>125,"EndAlpha"=>50,
            //),
        );
        $radarChart->drawRadar($this->dpImage, $this->dpData, $options); 
    }
}

class DrupalpScatterPlotChart extends DrupalpChart {
    protected $scatterPlot;
    
    function __construct($args) {
        parent::__construct($args);
        $this->set_data_points($args);
        $this->drawChart($args);
    }
    
    protected function set_data_points($args) {
        // for this beta implementation, we're going to assume a single series
        // to plot. We'll assume the first series is the x-axis and the second
        // series is the y-axis.
        
        $series = array_values($args['data']['series']);
        $names = array_keys($args['data']['series']);
        
        $this->dpData->addPoints($series[0], $names[0]);
        //$this->dpData->setAxisName(0,"X-Index");
        
        $this->dpData->setAxisXY(0,AXIS_X);
        $this->dpData->setAxisPosition(0,AXIS_POSITION_TOP);
       
        /* Create the Y axis and the binded series */
        $this->dpData->addPoints($series[1], $names[1]);
        $this->dpData->setSerieOnAxis($names[1],1);
        //$this->dpData->setAxisName(1,"Y-Index");
        $this->dpData->setAxisXY(1,AXIS_Y);
        $this->dpData->setAxisPosition(1,AXIS_POSITION_LEFT);
       
        /* Create the 1st scatter chart binding */
        $this->dpData->setScatterSerie($names[0],$names[1],0);
        $this->dpData->setScatterSerieDescription(0,"This year");
        //$this->dpData->setScatterSerieColor(0,array("R"=>0,"G"=>0,"B"=>255));
        $this->dpData->setScatterSeriePicture(0, $args['data']['icon'][0]);

    }
    
    protected function drawScale($args) {
        /* Draw the scale */
        
        $max_boundary = (float) $args['scale']['axis_boundaries']['Max'];
        $rows = 15;
        //$test_factor = (round(($max_boundary / 100), 0)) * 10;
        $factor = (round(($max_boundary / $rows) / 10)) * 10;
        $axis_boundaries = $args['scale']['axis_boundaries'];
        $axis_boundaries['Rows'] = $rows;
        $axis_boundaries['RowHeight'] = $factor;
        $scaleSettings = array(
            "Mode"=>SCALE_MODE_MANUAL,
            "ManualScale" => array(0 => $axis_boundaries, 1 => $axis_boundaries), //$args['scale']['axis_boundaries'], 1 => $args['scale']['axis_boundaries']),
            //"Factors"=>array($factor),
            "XMargin" => 15,
            "YMargin" => 15,
            "Floating" => TRUE,
            "GridR" => 80, "GridG" => 80, "GridB" => 80,
            "GridAlpha" => 100,
            "AxisAlpha" => 0,
            "TickAlpha" => 0,
            //"RemoveXAxis" => TRUE,
            "DrawSubTicks" => FALSE,
            "CycleBackground" => TRUE);
        
        $this->scatterPlot->drawScatterScale($scaleSettings);

    }
    
    protected function drawChart($args) {
        /* Create the pChart object */
        $this->dpImage = new pImage($args['settings']['size']['width'], $args['settings']['size']['height'],$this->dpData, TRUE);
        
        /* Turn of Anti-aliasing */
        $this->dpImage->Antialias = FALSE;
        
        /* Add a border to the picture */
        //$this->dpImage->drawRectangle(0,0,399,399,array("R"=>0,"G"=>0,"B"=>0));
        
        /* Set the default font */
        $font_name = 'calibri.ttf'; // $args['settings']['font-name'] . '.ttf';
        $font_size = $args['settings']['font-size'];
        $this->dpImage->setFontProperties(array(
            "FontName" => PCHART_FONT_DIRECTORY . $font_name,
            "FontSize" => $font_size,
            //"R" => 255, "G" => 255, "B" => 255, 
            //"Alpha" => 0,
            )
        ); 
        
        /* Set the graph area */
        $this->dpImage->setGraphArea(
            $this->graph_area['x'],
            $this->graph_area['y'],
            $this->graph_area['width'],
            $this->graph_area['height']); //1, 1, 390, 390); //40,40,370,370);
        
        /* Create the Scatter chart object */
        $this->scatterPlot = new pScatter($this->dpImage,$this->dpData);
        
        $this->drawScale($args);
        
        /* Draw a scatter plot chart */
        $this->dpImage->Antialias = TRUE;
        $this->scatterPlot->drawScatterPlotChart();
        //$this->scatterPlot->writeScatterLabel(array())

    }
}

class DrupalpBarChart extends DrupalpChart {
    
    function __construct($args) {
        parent::__construct($args);
        $this->set_data_points($args);
        $this->drawChart($args);
    }
    
    protected function set_data_points($args) {
        foreach ($args['data']['series'] as $name => $series) {
            $this->dpData->addPoints($series, $name);
        }
        
        // set abscissa
        $this->dpData->addPoints($args['data']['abscissa']['values'], $args['data']['abscissa']['name']);
        $this->dpData->setSerieDescription($args['data']['abscissa']['name'],$args['data']['abscissa']['name']);
        $this->dpData->setAbscissa($args['data']['abscissa']['name']);
        
        // set data axes
        $this->dpData->loadPalette(PCHART_PALETTE_DIRECTORY . $args['settings']['palette'], TRUE);
        
    }
    
    protected function drawScale($args) {
        /* Draw the chart scale */
        //$axisBoundaries = array(0=>array("Min"=>0, "Max"=>5));
        $scale_position = $args['chart']['type']['orientation'] == 'horizontal' ? SCALE_POS_TOPBOTTOM : SCALE_POS_LEFTRIGHT;
        $scale_config = array(
            "Mode"=>SCALE_MODE_MANUAL,
            "ManualScale" => array(0 => $args['scale']['axis_boundaries']), // $axisBoundaries,
            "Factors"=>array(1),
            "CycleBackground"=>FALSE,
            "DrawSubTicks"=>FALSE,
            "GridR"=>0,"GridG"=>0,"GridB"=>0,"GridAlpha"=>0, // fades the grid lines
            "AxisAlpha" => 0, // defaulting to no axis lines for now
            //"OuterSubTickWidth" => 12,
            "TickAlpha" => 0,
            "Pos"=>$scale_position,
            "XMargin" => $args['scale']['x-margin'],
            "YMargin" => $args['scale']['y-margin'],
        );
        $this->dpImage->drawScale($scale_config);
        
    }
    
    protected function drawChart($args) {
        
        $axis_position = $args['chart']['type']['orientation'] == 'horizontal' ? AXIS_POSITION_RIGHT : AXIS_POSITION_LEFT;
        $this->dpData->setAxisPosition(0, $axis_position);
        
        /* Create the pChart object */
        $this->dpImage = new pImage($args['settings']['size']['width'], $args['settings']['size']['height'], $this->dpData, TRUE); // 4th argument makes background transparent.
        
        //$this->dpImage->Antialias = FALSE;
        //$this->dpImage->drawGradientArea(0,0,500,300,DIRECTION_VERTICAL,array("StartR"=>240,"StartG"=>240,"StartB"=>240,"EndR"=>180,"EndG"=>180,"EndB"=>180,"Alpha"=>100));
        //$this->dpImage->drawGradientArea(0,0,500,300,DIRECTION_HORIZONTAL,array("StartR"=>240,"StartG"=>240,"StartB"=>240,"EndR"=>180,"EndG"=>180,"EndB"=>180,"Alpha"=>20));
        
        $font_name = $args['settings']['font-name'] . '.ttf';
        $font_size = $args['settings']['font-size'];
        $this->dpImage->setFontProperties(array(
            "FontName"=>PCHART_FONT_DIRECTORY . $font_name,
            "FontSize" => $font_size));
        
        $this->dpImage->setGraphArea($this->graph_area['x'], $this->graph_area['y'], $this->graph_area['width'], $this->graph_area['height']);

        /* Draw the chart scale */
        $this->drawScale($args);
        
         
        /* Turn on shadow computing */
        $this->dpImage->setShadow(TRUE,array("X"=>1,"Y"=>1,"R"=>0,"G"=>0,"B"=>0,"Alpha"=>10));
         
        /* Draw the chart */
        //$palette = array(
        //    "0"=>array("R"=>188,"G"=>224,"B"=>46,"Alpha"=>100),
        //    "1"=>array("R"=>224,"G"=>100,"B"=>46,"Alpha"=>100),
        //    "2"=>array("R"=>224,"G"=>214,"B"=>46,"Alpha"=>100),
        //    "3"=>array("R"=>46,"G"=>151,"B"=>224,"Alpha"=>100),
        //    "4"=>array("R"=>176,"G"=>46,"B"=>224,"Alpha"=>100),
        //    "5"=>array("R"=>224,"G"=>46,"B"=>117,"Alpha"=>100),
        //    "6"=>array("R"=>92,"G"=>224,"B"=>46,"Alpha"=>100),
        //);
        $palette = $args['settings']['color_override'];
        $chart_config = array(
            "DisplayPos"=>LABEL_POS_INSIDE,
            "DisplayValues"=>TRUE,
            "Rounded"=>TRUE,
            "Interleave"=>.1,
            "Surrounding"=>1,
            "OverrideColors" => $palette,
        );
        $this->dpImage->drawBarChart($chart_config);
    }
    
}