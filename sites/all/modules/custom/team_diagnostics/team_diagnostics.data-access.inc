<?php
// $Id$

/**
 * @file
 * Business logic and data access layer for Team Diagnostics
 */

/**
 *  Diagnostic Survey functions
 */
/**
 * Possible Refactor: Use Business Objects for data access?
class ResponseDetails
{
  public $response_id;
  public $item_id;
  public $points = 0;
  public $comments = '';
  
  function __construct() {
    $args = func_get_args();
    $num = func_num_args();
    if($num == 1) {
      $this->initialize_with_query($args[0]);
    }
  }
  
  public static function GetResponseDetails() {
    
  }
  
  function initialize_with_query($query_result) {
    $response_obj = $query_result->fetchObject();
    $this->points = $response_obj->points;
    $this->comments = $response_obj->comments;
  }
}
*/

function diagnostic_id_from_line_item($line_item_id) {
  $diag_sql = 'SELECT p.product_id FROM {commerce_line_item} li INNER JOIN {commerce_product} p ON li.line_item_label = p.sku WHERE li.line_item_id = :line_id';
  return db_query($diag_sql, array(':line_id' => $line_item_id))->fetchField();
}
function get_response_id($args) {
  // TODO: check to ensure $args contain a valid team_id, line_item_id, & member_email combination
  // return error if not
  
  $team_id = $args[0];
  $line_item_id = $args[1];
  $email = $args[2];
  
  //$diag_sql = 'SELECT p.product_id FROM {commerce_line_item} li INNER JOIN {commerce_product} p ON li.line_item_label = p.sku WHERE li.line_item_id = :line_id';
  //$diagnostic_id = db_query($diag_sql, array(':line_id' => $line_item_id))->fetchField();
  $diagnostic_id = diagnostic_id_from_line_item($line_item_id);
  
  $response_result = db_select('tptd_response', 'r')
    ->fields('r', array('response_id', 'finished'))
    ->condition('team_id', $team_id)
    ->condition('order_line_item_id', $line_item_id)
    ->condition('team_member_email', $email)
    ->execute();
  
  $response_id = 0;
  foreach($response_result as $response) {
    $response_id = $response->response_id;
    $finished = $response->finished;
    if($response_id && $finished) {
      throw new Exception(t('This assessment has been completed.'));
    }
  }
  
  //$diagnostic_from_line_item = db_select('commerce_line_item', 'li');
  //$diagnostic_from_line_item->join('commerce_product', 'p', 'li.line_item_label = p.sku');
  //$diagnostic_from_line_item->condition('li.line_item_id', $line_item_id);
  //$diagnostic_from_line_item->fields('p', array('product_id'));
  
  //$diagnostic_id = $diagnostic_from_line_item-
  
  
  if(!$response_id) {
    // insert response record and get response_id
    $response_id = db_insert('tptd_response')
      ->fields(array(
        'diagnostic_id' => $diagnostic_id, // TODO: Derive this from line_item_id
        'team_id' => $team_id,
        'order_line_item_id' => $line_item_id,
        'team_member_email' => $email,
        'started' => REQUEST_TIME,
      ))
      ->execute();
  }
  
  return $response_id;
}

function complete_response($response_id) {
  $num_updated = db_update('tptd_response')
    ->fields(array('finished' => REQUEST_TIME))
    ->condition('response_id', $response_id)
    ->execute();
}

// TODO: refactor these functions to return pages with variable numbers of items
function get_diagnostic_survey_item($item_info) {
  
  $page = $item_info['page'];
  $response_id = $item_info['response_id'];
  
  // for now, the page will be the equivalent of the item's sort number --mlp 2011.08.03
  $item_query = db_select('tptd_survey_items', 'i')
    ->condition('delta', $page)
    ->condition('diagnostic_id', $item_info['diagnostic_id'])
    ->fields('i', array('item_id', 'item_text', 'item_text_es'));
  
  $item_result = $item_query->execute();

  // Check language
  global $language;
  $lang = $language->language;
  
  $item_id = -1;
  $item_text = '';
  foreach($item_result as $row) {
    $item_id = $row->item_id;
    $item_text = $row->item_text;
    if ($lang == "es") {
      $item_text = $row->item_text_es;
    }
  }
  
  $option_query = db_select('tptd_item_options', 'o')
    ->condition('item_id', $item_id)
    ->fields('o', array('option_text', 'points'))
    ->orderBy('delta', 'ASC');
  
  $option_result = $option_query->execute();
  
  $options = array();
  foreach($option_result as $option_row) {
    $options[$option_row->points] = $option_row->option_text;
    if ($lang == "es") {
      // Few options, but many rows, so I'm translating here
      // instead of putting the translations into the DB.
      $option_trans = array(
        "Not at all Descriptive" => "Nada Descriptivo",
        "Seldom Descriptive" => "Raramente Descriptivo",
        "Somewhat Descriptive" => "Algo Descriptivo",
        "Often Descriptive" => "A Menudo Descriptivo",
        "Very Descriptive" => "Muy Descriptivo"
      );
      $options[$option_row->points] = $option_trans[$option_row->option_text];

    }



  }
  
  $response_detail = get_response_detail(array('response_id' => $response_id, 'item_id' => $item_id));
  
  $survey_item = array(
    'item_id' => $item_id,
    'text' => $item_text,
    'options' => $options,
    'response_value' => $response_detail['points'], // $response_points,
    'comments' => $response_detail['comments'], // $response_comments,
  );
  
  return $survey_item;
}

function get_response_detail($response_item = array()) {
  $response_id = $response_item['response_id'];
  $item_id = $response_item['item_id'];
  
  
  $response_info = db_select('tptd_response_details', 'd')
    ->fields('d', array('points', 'comments'))
    ->condition('response_id', $response_id)
    ->condition('item_id', $item_id)
    ->execute();
  
  //$response = $response_info->fetchObject(); //fetchAssoc();
  //$response = new ResponseDetail($response_info);
  
  $response_item['points'] = 0;
  $response_item['comments'] = '';
  foreach($response_info as $row) {
    $response_item['points'] = $row->points;
    $response_item['comments'] = $row->comments;
  }
  
  return $response_item;
  
}

function save_response_detail($user_response = array()) {
  $response_id = $user_response['response_id'];
  $item_id = $user_response['item_id'];
  $points = $user_response['item_points'];
  $comments = $user_response['comments'];
  
  $option_id = db_query('SELECT option_id FROM {tptd_item_options} WHERE item_id = :id AND points = :p', array(':id' => $item_id, ':p' => $points))->fetchField();
  
  $num_del = db_delete('tptd_response_details')
    ->condition('response_id', $response_id)
    ->condition('item_id', $item_id)
    ->execute();
    
  $ins_res = db_insert('tptd_response_details')
    ->fields( array(
      'response_id' => $response_id,
      'item_id' => $item_id,
      'option_id' => $option_id,
      'points' => $points,
      'comments' => $comments,
      'created' => REQUEST_TIME,
      )
    )
    ->execute();
}

function get_last_page_number($diagnostic_id) {
  
  return db_query('SELECT MAX(delta) FROM {tptd_survey_items} WHERE diagnostic_id = :did', array(':did' => $diagnostic_id))
    ->fetchField();
}

function get_available_diagnostic_count() {
    
    $result = get_available_recordset();
    
    $count = 0;
    foreach($result as $record) {
        $check_this = $record->line_item_id;
        $count++;
    }
    
    return $count;
}

function get_available_diagnostics() {
  
  $result = get_available_recordset();
  
  $available_diagnostics = array();
  foreach($result as $line_item) {
    $available_diagnostics[$line_item->line_item_id] = $line_item->title;
  }
  
  return $available_diagnostics;
}

function get_available_recordset() {
  global $user;
    
  $query = db_select('commerce_order', 'o');
  $query->join('commerce_line_item', 'li', 'o.order_id = li.order_id');
  $query->join('commerce_product', 'p', 'li.line_item_label = p.sku');
  $query->leftJoin('field_data_diagnostic_line_item_id', 'dlid', 'li.line_item_id = dlid.diagnostic_line_item_id_value');
  $query->condition('o.uid', $user->uid, '=');
  $query->condition('o.status', 'cart', '<>');
  $query->condition('o.status', 'checkout_checkout', '<>');
  $query->condition('o.status', 'checkout_review', '<>');
  $query->condition('dlid.diagnostic_line_item_id_value', null);
  $query->fields('li', array('line_item_id'));
  $query->fields('p', array('title'));

  return $query->execute();
}

function get_unfinished_responses($nid) {
  $query = db_select('field_data_team_email', 'e');
  $query->join('field_data_diagnostic_line_item_id', 'li', 'e.entity_id = li.entity_id');
  $query->leftJoin('tptd_response', 'r', 'e.team_email_value = r.team_member_email AND li.diagnostic_line_item_id_value = r.order_line_item_id');
  $query->condition('li.entity_id', $nid);
  $query->condition('r.response_id', null);
  $query->fields('li', array('diagnostic_line_item_id_value'));
  $query->fields('e', array('team_email_value'));
  
  return $query->execute(); //->fetchAssoc();
}

function get_team_responses($nid) {
    $query = db_select('field_data_team_email', 'm');
    $query->leftJoin('tptd_response', 'r', 'm.team_email_value = r.team_member_email AND m.entity_id = r.team_id');
    $query->condition('m.entity_id', $nid);
    $query->fields('r', array('diagnostic_id', 'started', 'finished'));
    $query->fields('m', array('team_email_value'));
    
    $result = $query->execute();
    
    
    $responses = array(
        'unfinished' => array(),
        'finished' => array(),
    );
    foreach($result as $record) {
        if($record->finished == null) {
            $responses['unfinished'][] = $record->team_email_value;
        }
        else {
            $responses['finished'][] = $record->team_email_value;
        }
    }
    
    return $responses;
    
}

function get_member_total_response_status($nid) {
  $sql = 'SELECT m.team_email_value, count(d.diagnostic_line_item_id_value) AS req_num, sum(if(r.response_id IS NULL, 0, 1)) AS num_complete
FROM field_data_team_email m
 LEFT JOIN field_data_diagnostic_line_item_id d ON m.entity_id = d.entity_id
 LEFT JOIN tptd_response r ON m.team_email_value = r.team_member_email AND d.diagnostic_line_item_id_value = r.order_line_item_id
WHERE m.entity_id = :entity_id
GROUP BY m.team_email_value';

  $param_array = array(':entity_id' => $nid);
  
  return get_array_from_sql($sql, $param_array);
  
}

function get_dashboard_info($nid, $uid = -1) {
  
  $dashboard_info = array();
  
  // first get info on applied diagnostics
  $sql = 'SELECT p.line_item_id, p.product_name AS diagnostic, count(m.team_email_value) AS member_count, sum(if(r.response_id IS NULL, 0, 1)) AS members_complete
FROM field_data_diagnostic_line_item_id d
 INNER JOIN tptd_vw_diagnostic_line_items p ON d.diagnostic_line_item_id_value = p.line_item_id
 LEFT JOIN field_data_team_email m on d.entity_id = m.entity_id
 LEFT JOIN tptd_response r ON m.team_email_value = r.team_member_email
 AND d.diagnostic_line_item_id_value = r.order_line_item_id
 AND r.finished IS NOT NULL
WHERE d.entity_id = :nid
GROUP BY p.line_item_id, p.product_name
ORDER BY 4';
  
  $param_array = array(':nid' => $nid);
  
  $dashboard_info['applied_info'] = get_array_from_sql($sql, $param_array);
  
  // then get available diagnostics
  if ($uid == -1) { // if uid wasn't passed into function...
    // ...get user_id from node...
    if($team_node = node_load($nid)) {
      $uid = $team_node->uid;
    } else { // ...and use the anonymous user if node doesn't load properly.
      $uid = 0;
    }
  }
  
  $sql = 'SELECT li.line_item_id, p.title AS diagnostic
FROM commerce_order o
 INNER JOIN commerce_line_item li ON o.order_id = li.order_id
 INNER JOIN commerce_product p ON li.line_item_label = p.sku
 LEFT JOIN field_data_diagnostic_line_item_id dlid ON li.line_item_id = dlid.diagnostic_line_item_id_value
WHERE o.uid = :uid AND o.status IN (\'pending\', \'complete\') AND dlid.diagnostic_line_item_id_value IS NULL';
  
  $param_array = array(':uid' => $uid);
  $dashboard_info['available_info'] = get_array_from_sql($sql, $param_array);
  
  return $dashboard_info;

}



function get_response_status($nid) {
  $sql = 'SELECT d.diagnostic_id, d.diagnostic_name,
  (SELECT count(team_email_value) FROM field_data_team_email WHERE entity_id = :nid) AS total_members,
  count(r.team_member_email) total_responses
FROM field_data_diagnostic_line_item_id li
 INNER JOIN tptd_vw_diagnostic_line_items d ON li.diagnostic_line_item_id_value = d.line_item_id
 LEFT JOIN tptd_response r ON r.order_line_item_id = li.diagnostic_line_item_id_value AND li.entity_id = r.team_id
WHERE li.entity_id = :nid
GROUP BY d.diagnostic_id, d.diagnostic_name';
  
  $param_array = array(':nid' => $nid);
  
  return get_array_from_sql($sql, $param_array);
}

function get_team_diagnostic_line_items($nid) {
  $sql = 'SELECT cli.line_item_id, p.product_id, p.title
   FROM {field_data_diagnostic_line_item_id} tli
    INNER JOIN {commerce_line_item} cli ON tli.diagnostic_line_item_id_value = cli.line_item_id
    INNER JOIN {commerce_product} p ON cli.line_item_label = p.sku
   WHERE tli.entity_id = :nid';

  $results = db_query($sql, array(':nid' => $nid));
  
  $team_line_items = array();
  foreach($results as $row) {
    $team_line_items[] = array(
      'product_id' => $row->product_id,
      'line_item_id' => $row->line_item_id,
      'diagnostic_name' => $row->title,
    );
  }
  
  return $team_line_items;
}

function get_array_from_sql($sql, $param_array) {
  $results = db_query($sql, $param_array);
  
  $results_array = array();
  while($row_array = $results->fetchAssoc()) {
    $results_array[] = $row_array;
  }
  
  return $results_array;
}

function get_team_survey_category_means($line_item_id) {
  $sql = 'SELECT p.diagnostic_id, p.diagnostic_name, i.category, avg(d.points) AS avg_of_points
FROM {tptd_diagnostics} AS p
 INNER JOIN {tptd_response} AS r ON p.diagnostic_id = r.diagnostic_id
 INNER JOIN {tptd_response_details} AS d ON r.response_id = d.response_id
 INNER JOIN {tptd_survey_items} AS i ON d.item_id = i.item_id
WHERE r.order_line_item_id = :line_item_id
GROUP BY p.diagnostic_id, p.diagnostic_name, i.category';
  
  $results = db_query($sql, array(':line_item_id' => $line_item_id));

  $category_means = array();
  
  foreach($results as $row) {
    $category_means['category'][] = $row->category;
    $category_means['mean_score'][] = $row->avg_of_points;
    
  }
  
  return $category_means;
  
}

function get_team_diagnostic_info_from_line_item($line_item_id) {
  $sql =
'SELECT dlid.entity_id AS team_id, n.title AS team_name,
 cp.title AS diagnostic_name, td.diagnostic_id, rd.entity_id AS report_id,
 n.uid
FROM {field_data_diagnostic_line_item_id} AS dlid
 INNER JOIN {node} AS n ON dlid.entity_id = n.nid
 INNER JOIN {commerce_line_item} AS cli ON dlid.diagnostic_line_item_id_value = cli.line_item_id
 INNER JOIN {commerce_product} AS cp ON cli.line_item_label = cp.sku
 INNER JOIN {field_data_field_related_diagnostic} AS rd ON cp.product_id = rd.field_related_diagnostic_product_id
 INNER JOIN {tptd_diagnostics} AS td ON cp.product_id = td.product_id
WHERE dlid.diagnostic_line_item_id_value = :line_item_id'; 

  $param_array = array(':line_item_id' => $line_item_id);
  
  $report_info = db_query($sql, $param_array)->fetchAssoc();

  // If the language is Spanish, translate the diagnostic title here

  // Check language
  global $language;
  $lang = $language->language;
  if ($lang == "es") {
      $titles = array(
        "Team Communication Profile" => "Perfil de Comunicación del Equipo",
        "Team Conflict Profile" => "Perfil de Conflicto de Equipo",
        "Team Meetings Profile" => "Perfil de las Reuniones de Equipo",
        "Team Development Survey" => "Encuesta de Desarrollo de Equipo",
        "Team Performance Survey" => "Encuesta de Rendimiento del Equipo"
      );

      $report_info['diagnostic_name'] = $titles[$report_info['diagnostic_name']];

  }

  return $report_info;

}

function get_team_line_item_diagnostic_name_from_nid($nid) {
  $sql = 'SELECT li.line_item_id, p.title AS diagnostic
          FROM commerce_order o
           INNER JOIN commerce_line_item li ON o.order_id = li.order_id
           INNER JOIN commerce_product p ON li.line_item_label = p.sku
           INNER JOIN field_data_diagnostic_line_item_id dlid ON li.line_item_id = dlid.diagnostic_line_item_id_value
          WHERE dlid.entity_id = :nid';
  
  $params = array(':nid' => $nid);
  
  $result = db_query($sql, $params);
  $diagnostic_line_items = array();
  foreach ($result as $record) {
    $diagnostic_line_items[$record->line_item_id] = $record->diagnostic;
  }
  
  return $diagnostic_line_items;
}

function get_user_teams($uid) {
  $sql = "SELECT nid, title
          FROM node
          WHERE type = 'team' AND uid = :uid";
          
  $params = array(':uid' => $uid);
  
  return get_array_from_sql($sql, $params);
}

function user_completed_orders_count($uid) {
  $sql = "SELECT count(order_id)
FROM commerce_order 
WHERE status IN ('pending','processing','completed') AND uid = :uid";
 
 $param_array = array(':uid' => $uid);
 
 return db_query($sql, $param_array)->fetchField();
 
}