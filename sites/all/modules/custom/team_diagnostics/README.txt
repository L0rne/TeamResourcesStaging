
[TCPDF Hack] Now obsolete--thankfully--after we replaced TCPDF with PDFLib
Added this code block to tcpdf.php (at approximate line 7338) in order to hack
the issue with superimposing a Google Chart image over an image background.
(See Ticket #49 in Trac.)
// check if we are passing an image as file or string
if (substr($file, 0, 28) == 'http://chart.apis.google.com') {
    $position_qry = '&position='; //len = strlen('$position=');
    $position = explode('x', substr($file, strpos($file, $position_qry) + strlen($position_qry)));
    $chart_url = substr($file, 0, strpos($file, $position_qry));
    
    if (isset($position[0]) && isset($position[1])) {
        $x = $position[0];
        $y = $position[1];
    }
    
    $file = get_google_chart_data_stream($file);
}

This calls the function get_google_chart_data_stream() that encodes the chart
URL and uses fopen() to create a local copy to position over the background
image.

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

In order to set the position, a query parameter needs to be tacked on to the end
of the Chart URL:

For example:

$chart_url = chart_url($chart);
return $chart_url . '&position=16x25';

Currently, the position parameter passes the PDF position, not the CSS position.
Eventually, we will want to convert CSS to PDF units.


[Chart API Hack]
The API does not include a setting for scale range, so I added this to the
switch statement in the _chart_append($attr, $value, &$data) function.

  // Data Range (added by mlp on 2011-08-17)
    case 'chds':
      $data[$attr] = implode(',', $value);
      break;
      
and this line to the chart_build($chart) function:

_chart_append('chds',  $chart['#scale_range'],             $data);

Syntax use this new setting:

$chart['#scale_range'] = array(0,5); // this will set the x-axis range to 0 - 5.


[DataGrid Queries]
Base query:
SELECT 
FROM tptd_response_details AS d 
 INNER JOIN tptd_survey_items AS i ON d.item_id = i.item_id 
 INNER JOIN tptd_response AS r ON d.response_id = r.response_id 
WHERE r.order_line_item_id = :line_item_id 
GROUP BY r.response_id

[Test Survey Links]
http://teambenchmark/team-diagnostic/survey/12/1/pmac@triaxiapartners.com
http://teambenchmark/team-diagnostic/survey/12/13/llamport@yahoo.com
http://teambenchmark/team-diagnostic/survey/12/13/alex.porlier@yahoo.com
http://teambenchmark/team-diagnostic/survey/12/13/pmac@triaxiapartners.com
http://teambenchmark/team-diagnostic/survey/12/12/llamport@yahoo.com
http://teambenchmark/team-diagnostic/survey/12/12/alex.porlier@yahoo.com
http://teambenchmark/team-diagnostic/survey/12/12/pmac@triaxiapartners.com
TEST
