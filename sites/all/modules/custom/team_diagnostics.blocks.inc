<?php
// $Id$

/**
 * @file
 * Implements functionality for team blocks
 *
 * 
 */
require_once(dirname(__FILE__) . '/team_diagnostics.data-access.inc');

function team_diagnostics_block_info() {
  $blocks = array();
  
  $blocks['survey_status'] = array(
    'info' => t('Survey Status'),
    'cache' => DRUPAL_NO_CACHE,
  );
  
  $blocks['available_diagnostics'] = array(
    'info' => t('Available Diagnostics'),
    'cache' => DRUPAL_NO_CACHE,
  );
  
  $blocks['send_email_to_team'] = array(
    'info' => t('Send Invitations'),
    'cache' => DRUPAL_NO_CACHE,
  );
  
  $blocks['download_diagnostic_reports'] = array(
    'info' => t('Diagnostic Downloads'),
    'cache' => DRUPAL_NO_CACHE,
  );
  
  $blocks['distribution_list'] = array(
    'info' => t('Team Distribution List'),
    'cache' => DRUPAL_NO_CACHE,
  );
  
  $blocks['diagnostic_dashboard'] = array(
    'info' => t('Diagnostic Dashboard'),
    'cache' => DRUPAL_NO_CACHE,
  );
  
  return $blocks;
}

function team_diagnostics_block_view($delta = '') {
  
  $nid = 0;
  $block = array();
  
  if (arg(0) == 'node' && is_numeric(arg(1))) {
    $nid = arg(1);
  }
  if (!$nid) {
    return $block;
  }
  
  $node = node_load($nid);
  
  switch($delta) {
    case 'available_diagnostics':
      $block['subject'] = 'Available Diagnostics';
      $block['content'] = drupal_get_form('available_diagnostics_form');
      break;
    
    case 'survey_status':
      $block['subject'] = 'Survey Status';
      $block['content'] = _team_get_responses($nid); // 'Not quite done yet...';
      break;
    
    case 'send_email_to_team':
      $block['subject'] = 'Email Team';
      $block['content'] = drupal_get_form('send_email_form');
      break;
    
    case 'download_diagnostic_reports':
      $block['subject'] = 'Download Reports';
      $block['content'] = _get_report_links($nid);
      break;
    
    case 'distribution_list':
      $block['subject'] = 'Team Members';
      $block['content'] = drupal_get_form('add_team_email_form', $node); // _get_team_distribution($nid);
      break;
    
    case 'diagnostic_dashboard':
      $block['subject'] = 'Diagnostic Dashboard';
      $block['content'] = drupal_get_form('diagnostic_dashboard_form');
      break;
  }
  
  return $block;
}

function send_email_form($form, &$form_state) {
  global $user;
  
  $nid = 0;
  if (arg(0) == 'node' && is_numeric(arg(1))) {
    $nid = arg(1);
  }
  /*
  if (!$nid) {
    return $form;
  }
  */
  
  $form['#prefix'] = '<div id="send-team-email-form">';
  $form['#suffix'] = '</div>';
  
  $form['node_id'] = array(
    '#type' => 'hidden',
    '#value' => $nid,
    '#prefix' => '<div id="send-to-node-id">',
    '#suffix' => '</div>',
  );
  
  // ui-helper-reset ui-state-default ui-corner-all
  $form['email_team'] = array(
    '#prefix' => '<h3 id="email-team-header" class="ui-accordion-header">&nbsp;</h3><div id="email-team-container" class="ui-helper-reset ui-widget-content ui-corner-all">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
  );
  
  $form['email_team']['send_button'] = array(
    '#type' => 'submit',
    '#ajax' => array(
      'callback' => 'ajax_send_team_email_callback',
      'wrapper' => array('send-email-form'), //'send-email-message', 'send-to-node-id', 'send-team-email-button'), //'send-team-email-form'),
      //'name' => 'send-team-email',
    ),
    '#value' => t('Send'),
    '#prefix' => '<div id="send-team-email-button">',
    '#suffix' => '</div><br/>',
  );
  
  $two_weeks = mktime(0, 0, 0, date("m"), date("d")+14, date("y"));
  $default_date = date('Y-m-d H:i:s', $two_weeks);
  $form['email_team']['due_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('Due Date'),
    '#date_format' => 'm/d/Y',
    '#default_value' => $default_date,
    '#date_year_range' => '0:+1',
    '#date_label_position' => 'none',
    '#size' => 20,
    '#maxlength' => 20,
    '#required' => TRUE
  );
  
  
  $invite_msg = t('Send your team an invitation or reminder. We will append the following instructions and survey links to the end of each email message.');
  $boilerplate = team_diagnostics_get_team_invite_boilerplate();
  //die(var_dump($user));
  $boilerplate = preg_replace('/\[admin:email\]/', $user->mail, $boilerplate);
  $invite_msg .= '<br/><br/>' . $boilerplate;
  $form['email_team']['email_message'] = array(
    '#title' => t('Message (optional)'),
    '#type' => 'textarea',
    '#description' => $invite_msg,
    '#default_value' => '',
    '#prefix' => '<div id="send-email-message">',
    '#suffix' => '</div>',
    '#required' => FALSE,
    //'#element_validate' => array('new_email_validate'),
  );
  
  return $form;
}

function send_email_form_submit($form, &$form_state) {

  global $user;
  global $base_url;
  
  $from = $user->mail;
  $msg = $form_state['values']['email_team']['email_message'];
  $nid = $form_state['values']['node_id'];
  $subject = t('An Invitation to the !team Team Assessment', array('!team' => $team_node->title));
  
  $unfinished_result = get_unfinished_responses($nid);
  $team_node = node_load($nid);
  
  $output = '';
  $msg_count = 0;
  $msg_type = 'status';
  $msg_params = array();
  $msg_params['admin_email'] = $user->mail;
  $msg_params['due_date'] = $form_state['values']['email_team']['due_date'];
  $msg_params['team_name'] = $team_node->title;
  $diagnostic_items = get_team_line_item_diagnostic_name_from_nid($nid);
  
  foreach($unfinished_result as $send_this) {
    $to = $send_this->team_email_value;
    $line_item_id = $send_this->diagnostic_line_item_id_value;
    
    // build survey link
    $link = $base_url . '/team-diagnostic/survey/' . $nid . '/' . $line_item_id . '/' . $to;
    $msg_params['link'] = $link;
    $msg_params['diagnostic_name'] = $diagnostic_items[$line_item_id];
    if (!send_team_email($to, $from, $subject, $msg, $msg_params)) {
      $msg_type = 'error';
      $output .= "Error sending to $to<br />";
      //$output .= $link . '<br />';
    }
    else {
      $msg_count++;
      
    }
    
  }
  $message_sent_text = $msg_count == 1 ? '1 Message' : $msg_count . ' Messages';
  $output = "<h3>$message_sent_text Sent!</h3>" . $output;
  $message_clarification = t('If you have more than one diagnostic applied to this team, a separate email was sent to each member for each diagnostic.');
  $output .= "<br>($message_clarification)";
  drupal_set_message($output, $msg_type); // t("Email messages sent. Instructions and links were appended to your message.")); //
  //return $form;
}

function ajax_send_team_email_callback($form, $form_state){
  
  $form['email_team']['email_message']['#value'] = '';
  $form_state['rebuild'] = TRUE;
  return $form;
  
  /*
  $selector = '#email-team-header';
  $commands = array();
  $commands[] = ajax_command_invoke($selector, 'team_diagnostics');
  return array('#type' => 'ajax', '#commands' => $commands);
  */
}

function send_team_email($to, $from, $subject, $admin_message, $msg_params) {
  $module = 'team_diagnostics';
  $key = 'team_message';
  $language = language_default();
  $params = array();
  $from = NULL;
  $send = FALSE;
  $message = drupal_mail($module, $key, $to, $language, $params, $from, $send);

  $boilerplate = team_diagnostics_get_team_invite_boilerplate();
  $boilerplate = preg_replace('/\[survey link\]/', $msg_params['link'], $boilerplate);
  $boilerplate = preg_replace('/\[admin:email\]/', $msg_params['admin_email'], $boilerplate);
  $boilerplate = preg_replace('/\[Due Date\]/', $msg_params['due_date'], $boilerplate);
  $boilerplate = preg_replace('/\[diagnostic name\]/', $msg_params['diagnostic_name'], $boilerplate);
  $boilerplate = preg_replace('/\[team name\]/', $msg_params['team_name'], $boilerplate);
  
  $message['subject'] = $subject;
  $message['body'] = array();
  $message['body'][] = $admin_message . '<br/><br/><br/>.................................';
  $message['body'][] = $boilerplate;
  //$message['body'][] = $link;

  // Retrieve the responsible implementation for this message.
  $system = drupal_mail_system($module, $key);

  // Format the message body.
  $message = $system->format($message);

  // Send e-mail.
  $message['result'] = $system->mail($message);
  $result = $message['result'];
  watchdog('team_diagnostics', "Message send from $from to $to with result $result for $link");
  
  return $message['result'];
}

function team_diagnostics_get_team_invite_boilerplate() {
  $bp_path = explode('/', drupal_get_normal_path('internal/team-invite-boilerplate'));
  $bp_id = $bp_path[1];
  $bp_node = node_load($bp_id);
  
  return $bp_node->body[$bp_node->language][0]['value'];
}

function available_count_message() {
  $diagcount = get_available_diagnostic_count();
  $diagnostic_sg_pl = $diagcount == 1 ? 'diagnostic' : 'diagnostics';
  return t("You have $diagcount available $diagnostic_sg_pl.");
}

function diagnostic_dashboard_form($form, &$form_state) {
  drupal_add_library('system', 'ui.accordion');
  drupal_add_js('jQuery(document).ready(function() { jQuery("#dashboard-accordion").accordion(); });', 'inline');
  drupal_add_js(drupal_get_path('module', 'team_diagnostics') . '/team.js');
  
  $nid = 0;
  if (arg(0) == 'node' && is_numeric(arg(1))) {
    $nid = arg(1);
  }
  if (!$nid) {
    return $form;
  }
  $form['outer_wrapper'] = get_dashboard_form_items($nid);
  $form['outer_wrapper']['#prefix'] = '<div id="team-diagnostic-dashboard-form">';
  $form['outer_wrapper']['#suffix'] = '</div>';
  $form['outer_wrapper']['#tree'] = TRUE;
  
  return $form;
}

function get_dashboard_form_items($nid) {
  global $user;
  $form = array();
  $diagnostic_info = get_dashboard_info($nid, $user->uid);
  
  $unused = count($diagnostic_info['available_info']);
  $completed = 0;
  foreach($diagnostic_info['applied_info'] as $delta => $applied_diagnostic) {
    $member_count = (float) $applied_diagnostic['member_count'];
    $members_complete = (float) $applied_diagnostic['members_complete'];
    if ($members_complete == $member_count) {
      $completed++;
    }
  }
  
  $purchase_more_link = l('Purchase Additional Diagnostics', 'diagnostic-products');
  //TODO: Info Header Item
  
   
  $info_markup =  "<div id='info-header-text'>" .
                  "    <div id='info-header-unused-diagnostics'>" .
                  "        <span id='info-unused-text'>Unused&nbsp;Diagnostics:</span>&nbsp;" .
                  "        <span id='info-unused-count'>$unused</span>" .
                  "    </div>" .
                  "    <div>" .
                  "        <span id='info-completed-text'>Completed&nbsp;Diagnostics:</span>&nbsp;" .
                  "        <span id='info-completed-count'>$completed</span>" .
                  "    </div>" .
                  "</div>" .
                  "<div id='info-header-purchase'>" .
                  "    <a id='info-header-purchase-link' href='/diagnostic-products' title='Purchase another diagnostic'>&nbsp;</a>" .
                  "</div>";
  $form['info_header'] = array(
    '#prefix' => '<div id="dashboard-info-header" class="ui-helper-reset ui-widget-content ui-corner-all">',
    '#suffix' => '</div>',
    '#markup' => $info_markup,
  );
  
  /*
  $directions_path = explode('/', drupal_get_normal_path('directions'));
  $directions_id = $directions_path[1];
  $directions_node = node_load($directions_id);
  $form['directions'] = array( //<a href="#" id="directions_btn">
    '#prefix' => '<div id="dashboard-inner"><h3 id="directions-header" class="team-sidebar-header-thingy ui-accordion-header ui-helper-reset ui-state-default ui-corner-all"><span class="ui-icon"></span>' . $directions_node->title . '</h3><div id="diagnostic-directions" class="ui-helper-reset ui-widget-content ui-corner-all">',
    '#suffix' => '</div>',
    '#markup' => $directions_node->body[$directions_node->language][0]['value'],
    //'#weight' => -1,
  );
  */
  
  $form['directions'] = array(
    '#prefix' => '<div id="dashboard-inner"><h3 id="directions-header" class="team-sidebar-header-thingy ui-accordion-header ui-helper-reset ui-state-default ui-corner-all"><span class="ui-icon"></span><a class="colorbox-node" href="/directions?width=960&height=500">Directions</a></h3><div id="diagnostic-directions" class="ui-helper-reset ui-widget-content ui-corner-all">',
    '#suffix' => '</div>',
    '#markup' => '<a class="colorbox-node" href="/directions?width=960&height=500">View Directions Page.</a>',
  );
  
  $form['dashboard'] = array(
    '#prefix' => '<div id="dashboard-accordion">',
    '#suffix' => '</div></div>',
  );
  
  
  $instructions = '<h5>Viewing Your Report</h5><p>When your team has completed 75% or more of the survey, the report PDF will become available to view. A "View Report" button will appear below, on top of the the progress chart.</p>';
  
  foreach($diagnostic_info['applied_info'] as $delta => $applied_diagnostic) {

    $item_id = 'applied_item_' . $delta;
    $diagnostic_name = $applied_diagnostic['diagnostic'];
    $form['dashboard'][$item_id] = array(
      '#prefix' => "<h3 class='team-sidebar-header-thingy'>$diagnostic_name</h3><div id='dashboard-item-$delta' class='diagnostic-status'>",
      '#suffix' => "</div>",
      '#tree' => TRUE,
    );
    
    $form['dashboard'][$item_id]['instructions'] = array(
      '#markup' => $instructions,
    );
    
    $member_count = (float) $applied_diagnostic['member_count'];
    $members_complete = (float) $applied_diagnostic['members_complete'];
    $percent = '0';
    if ($member_count > 0) {
      $percent = ceil(($members_complete / $member_count) * 100);
    }
    
    $download = $percent > 10; //74;
    $percent .= '%';
    
    $form['dashboard'][$item_id]['details'] = array(
      '#prefix' => '<div class="diagnostic-details">',
      '#suffix' => '</div>',
      '#markup' => "<h5>Diagnostic Details</h5><span class='progress-text'>Progress:</span>&nbsp;<span class='percent-complete'>$percent</span>",
    );
    
    $form['dashboard'][$item_id]['chart_download'] = array(
      '#prefix' => "<div class='chart-report-download'>",
      '#suffix' => "</div>",
      '#tree' => TRUE,
    );
    
    $progress_chart_classes = $members_complete > 0 ? "progress-chart" : "progress-chart not-started";
    $form['dashboard'][$item_id]['chart_download']['progress_chart'] = array(
      '#prefix' => "<div class=\"$progress_chart_classes\">",
      '#suffix' => '</div>',
      '#markup' => get_progress_chart($delta, $applied_diagnostic),
    );
    
    if ($download) {
      $attributes = array( 'attributes' => array(
        'title' => 'Download the PDF',
        'target' => '_blank',
        ),
        'html' => true,
      );
      $line_item_id = $applied_diagnostic['line_item_id'];
      $report_link = l('<img src="/sites/teambenchmark/themes/benchmark/images/view-report-btn.png" alt="View Report" />', "team-diagnostic/report/$line_item_id", $attributes);
      
      $form['dashboard'][$item_id]['chart_download']['download'] = array(
        '#prefix' => '<div class="download-link">',
        '#suffix' => '</div>',
        '#markup' => $report_link,
      );
    }
  }
  
  $appy_path = explode('/', drupal_get_normal_path('apply-diagnostic-warning'));
  $appy_id = $appy_path[1];
  $appy_node = node_load($appy_id);
  $apply_content = $appy_node->body[$appy_node->language][0]['value'];
  
  foreach($diagnostic_info['available_info'] as $delta => $available_diagnostic) {
    
    $line_item_id = $available_diagnostic['line_item_id'];
    $item_id = 'available_item_' . $delta;
    $diagnostic_name = $available_diagnostic['diagnostic'];
    
    $apply_link = "team/$nid/nojs/apply/$line_item_id/$delta"; // "team/$nid/nojs/apply/$line_item_id/$delta"
    $diag_button = team_ajax_text_button(t(''), $apply_link, t('Apply to this team'));
    
    
    $apply_text = preg_replace('/\[apply_item_link\]/', $diag_button, $apply_content); //$apply_link, $apply_content);
    
    $form['dashboard'][$item_id] = array(
      '#type' => 'markup',
      '#prefix' => "<h3 class='team-sidebar-header-unused'>$diagnostic_name&nbsp;</h3><div id='available-row-$delta' class='available-row'>",
      '#suffix' => "</div>",
      '#markup' => $apply_text, //"$diag_button",
    );
    
  }
  
  return $form;
}

function get_progress_chart($delta, $diagnostic) {
  $chart_id = 'chart_' . $delta;
  $chart = array(
    '#chart_id' => $chart_id,
    '#title' => '', // chart_title($diagnostic['diagnostic'], '000000', 11),
    '#type' => CHART_TYPE_PIE_3D,
    '#size' => chart_size(180, 120),
    '#chart_fill' => array('bg','s',67676700),
    '#data_colors' => array('68BAEB', '777777'),
    '#scale_range' => array(),
  );
  
  // calculate pie data
  $unfinished = (int) $diagnostic['member_count'] - (int) $diagnostic['members_complete'];
  $finished = (int) (int) $diagnostic['members_complete'];
  $chart['#data'] = array($finished,$unfinished);
  
  return theme_chart(array('#chart_id' => $chart_id, 'chart' => $chart));
}

function available_diagnostics_form($form, &$form_state) {
  
  $nid = 0;
  if (arg(0) == 'node' && is_numeric(arg(1))) {
    $nid = arg(1);
  }
  if (!$nid) {
    return $form;
  }
  
  $availmsg = available_count_message();
  
  $available_diagnostics = get_available_diagnostics(); // array('Team Survey','Team Profile');
  
  $rows = array();
  $row_count = 0;
  foreach($available_diagnostics as $delta => $diagnostic_name) {
    //$email = $email_object['safe_value'];
    $row = array(
      'id' => 'available-row-' . $row_count,
      'class' => array('available-row-' . $delta),
      'data' => array(
                team_ajax_text_button(t('+'), "team/$nid/nojs/apply/$delta/$row_count", t('Apply to this team')),
                $diagnostic_name,
      ),
    );
      
    $rows[] = $row;
    $row_count++;
  }
  
  $header = array(t('Add'), t('Diagnostic'));
  
  $tablevars = array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'available-diagnostics-table'),
    'caption' => $availmsg,
    'colgroups' => NULL,
    'sticky' => 0,
    'empty' => NULL,
  );
  
  $form['available_diagnostics'] = array(
    '#markup' => theme_table($tablevars),
    '#prefix' => '<div>',
    '#suffix' => '</div>',
  );
  
  return $form;
}

function apply_diagnostic($nid, $js, $line_item_id, $row) {
  
  // apply a diagnostic
  $node = node_load($nid);
  $node->diagnostic_line_item_id[$node->language][]['value'] = $line_item_id;
  node_save($node);
  $availmsg = available_count_message();
  if($js) {
    
    $row_to_delete = '#available-row-' . $row;
    $commands = array();
    
    $dashboard_form = get_dashboard_form_items($nid);
    $dashboard_html = drupal_render($dashboard_form); //['dashboard']);
    $commands[] = ajax_command_html('#team-diagnostic-dashboard-form', $dashboard_html);
    $commands[] = ajax_command_invoke('#dashboard-accordion', 'accordion');
    $commands[] = ajax_command_invoke('#diagnostic-directions', 'hide');
      
    $page = array('#type' => 'ajax', '#commands' => $commands);
    ajax_deliver($page);
    return;
  }
  else {
    $output = t("Put a confirmation page here.");
    return $output;;
  }
  
}

function _team_get_responses($nid) {
  
  $items = array();
  
  $response = get_response_status($nid);
  
  foreach($response as $delta => $diagnostic) {
    $chart_id = 'chart_' . $delta;
    $chart = array(
      '#chart_id' => $chart_id,
      '#title' => chart_title($diagnostic['diagnostic_name'], '000000', 11),
      '#type' => CHART_TYPE_PIE,
      '#size' => chart_size(120, 100),
      '#chart_fill' => array('bg','s',67676700),
      '#scale_range' => array(),
    );
    //$chart['#title'] = chart_title($diagnostic['diagnostic_name'], '000000', 11);
    
    // calculate pie data
    $unfinished = (int) $diagnostic['total_members'] - (int) $diagnostic['total_responses'];
    $finished = (int) (int) $diagnostic['total_responses'];
    $chart['#data'] = array($finished,$unfinished);
    
    
    $chart_image = theme_chart(array('#chart_id' => $chart_id, 'chart' => $chart));
    
    $items[] = array(
      '#type' => 'markup',
      '#markup' => "<div id='team-status-$delta' class='status-image'>$chart_image</div>",
    );
    
    $chart = array();
  }
  
  return $items;
}

function _get_report_links($nid) {
  $diagnostic_list = get_team_diagnostic_line_items($nid);
  
  $attributes = array( 'attributes' => array(
    'title' => 'Download the PDF',
    'target' => '_blank',
    ),
  );
  
  $report_links = array(
    'title' => '',
    'type' => 'ul',
    'items' => array(),
    'attributes' => array(),
  );
  foreach($diagnostic_list as $delta => $diagnostic) {
    $diagnostic_name = $diagnostic['diagnostic_name'];
    $line_item_id = $diagnostic['line_item_id'];
    $report_links['items'][] = l($diagnostic_name, "team-diagnostic/report/$line_item_id", $attributes);
  }
  
  $items = array();
  $items[]['#markup'] = theme_item_list($report_links);
  
  return $items;
}

function _get_team_distribution($nid) {
  
  $items = array();
  
  $items[]['#markup'] = '[Team Distribution List will go here]';
  
  return $items;
}

function add_team_email_form($form_state, $node) {
  //$form['action']['submit']['#ajax']
  $node_base = $node['build_info']['args'][0];
  
  $form['#prefix'] = '<div id="team-email-distribution-form">';
  $form['#suffix'] = '</div>';
  
  $todo_text = t('TODO: Add team member emails using the form below:');
  $list_output = '<div class="team-instruction">TODO: Add team member emails using the form below:</div>';
  
  $member_response = get_member_total_response_status($node_base->nid);
  $max_members = $node_base->field_max_members[$node_base->language][0]['value'];
  $member_count = count($member_response);
  $list_output = get_team_distribution_widget($node_base->nid, $member_response);
  
  $prefix = '<div id="team-email-distribution-list"';
  $prefix .= $member_count > 0 ?  '>' : ' class="members-empty">';
  
  $form['email_list'] = array(
    '#markup' => $list_output,
    '#prefix' => $prefix,
    '#suffix' => '</div>',
    '#weight' => 1,
  );
  
  
  
    // For convenience, save the node id.
  $form['nid'] = array( 
    '#type' => 'value', 
    '#value' => $node_base->nid,
  );
  
  $form['team_member_add'] = array(
    '#title' => t('Team Members/Email <img src="/misc/arrow-asc.png" />'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => -1,
  );
  // ui-helper-reset ui-state-default ui-corner-all t('Add Team Members')
  $form['team_member_add'] = array(
    '#prefix' => '<h3 id="add-team-header" class="ui-accordion-header">&nbsp;</h3><div id="add-team-container" class="ui-helper-reset ui-widget-content ui-corner-all">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
  );
  
  $msg = t("Using !count of !max available member addresses", array('!count' => $member_count, '!max' => $max_members));
  $form['team_member_add']['user_message'] = array(
    '#markup' => '<div id="team-node-team-count-label">' . $msg . '</div>',
  );
  
  $form['team_member_add']['new_email'] = array(
    '#title' => t("Enter your team's email distribution list"),
    '#type' => 'textarea',
    '#description' => t('You can enter one email per line or separate the addresses with commas.'),
    '#default_value' => '',
    '#prefix' => '<div id="new-email-field">',
    '#suffix' => '</div>',
    '#required' => TRUE,
    '#element_validate' => array('new_email_validate'),
  );
  


  $form['team_member_add']['add_button'] = array(
    '#type' => 'button',
    '#ajax' => array(
      'callback' => 'ajax_add_team_email_callback',
      'wrapper' => array('team-email-distribution-form'), //, 'new-email-field'),
      'name' => 'add1',
    ),
    '#value' => t('Add Team Member Email Addresses'),
  );
  
  return $form;
}

function get_team_distribution_widget($nodeid, $team_email_list) {
  
  //$headers = array('Email', 'Remove');
  $rows = array();
  
  foreach($team_email_list as $delta => $email_object) {
    $email = $email_object['team_email_value'];
    $complete = (int) $email_object['num_complete'];
    //$required = (int) $email_object['req_num'];
    $remove_button = '<img src="/sites/teambenchmark/themes/benchmark/images/started-btn-grey.png" title="This member has already started or finished a diagnostic survey" alt="Member has taken a survey" />';
    if ($complete == 0) { // == $required) {
      $remove_button = team_ajax_text_button('<img src="/sites/teambenchmark/themes/benchmark/images/remove-btn-red.png" />', "team/$nodeid/nojs/delete/$email/$delta", t('Remove this team member'));
    }
    //$email = $email_object['safe_value'];
    $row = array(
      'id' => 'row' . $delta,
      'class' => array('team-email-row-' . $delta),
      'data' => array(
                array('class' => 'team-email', 'data' =>$email),
                array('class' => 'email-action-button', 'data' =>$remove_button),
      ),
    );
      
    $rows[] = $row;
  }
  
  $header = array(); //(t('Team Member Email'), t('X'));
  
  $tablevars = array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'team-email-table'),
    'caption' => '', //'Team Distribution List',
    'colgroups' => NULL,
    'sticky' => 0,
    'empty' => NULL,
  );
  
  return theme_table($tablevars);
}

function new_email_validate($element, &$form_state) {
  
  $node = node_load($form_state['values']['nid']);
  $emailvalues = get_email_array($element['#value']);
  $current_member_count = count($node->team_email[$node->language]);
  $max_members = $node->field_max_members[$node->language][0]['value'];
  
  if ( (count($emailvalues) + $current_member_count) > $max_members) {
    $msg = t('You have exceeded the maximum of !max members for this team.', array('!max' => $max_members));
    form_error($element, $msg);
    $emailvalues = array();
    $element['#value'] = '';
    $form_state['values']['new_email'] = '';
    return $element;
  }
  
  
  foreach($emailvalues as $delta => $email) {
    if(!valid_email_address($email)) {
      $msg = t('!email is not a valid email address.', array('!email' => $email));
      form_error($element, $msg);
      //return $element;
      unset($emailvalues[$delta]);
    }
  }
  $element['#value'] = implode(",", $emailvalues);
  $form_state['values']['new_email'] = implode(",", $emailvalues);
  return $element;
}

function get_email_array($emailstring) {
  $emailvalues = preg_split("/[\n]|\,/", $emailstring);
  foreach($emailvalues as $delta => $email) {
    $emailvalues[$delta] = str_replace(' ', '', $email);
  }
  return $emailvalues;
}

function ajax_add_team_email_callback($form, $form_state) {
  
  $emailvalues = get_email_array($form_state['values']['new_email']);
  $emails = array();
  foreach($emailvalues as $delta => $email) {
    $emails[] = array(
      'value' =>  $email, //$form_state['values']['new_email'],
      'format' => null,
      'safe_value' => $email,
    );
  }
  
  $node = node_load($form_state['values']['nid']);
  
  //check for duplicate emails
  foreach($emails as $email_array) {
    if(in_array($email_array, $node->team_email['und'])) {
      $element = $form['new_email'];
      $msg = t('!email has already been added.', array('!email' => $email_array['safe_value']));
      form_error($element, $msg); //, FALSE);
    }
    else {
      // insert an email
      if(valid_email_address($email_array['safe_value'])) {
        $node->team_email['und'][] = $email_array;
      }
    }
  }
 
  // validate, submit, and save 
  node_validate($node, $form, $form_state);
  $node = node_submit($node);
  node_save($node);
  
  $member_response = get_member_total_response_status($node->nid);
  $output = get_team_distribution_widget($node->nid, $member_response);
  //$output = get_team_distribution_widget($node->nid, $node->team_email['und']);
  
  $max_members = $node->field_max_members[$node->language][0]['value'];
  $member_count = count($member_response);
  $msg = t("Using !count of !max available member addresses", array('!count' => $member_count, '!max' => $max_members));
  
  $form['email_list']['#markup'] = $output;
  $form['team_member_add']['new_email']['#value'] = '';
  $form['team_member_add']['user_message']['#markup'] = '<div id="team-node-team-count-label">' . $msg . '</div>';
  return $form; //['email_list']; // $elements;
  
  
}

function team_email_delete($nid, $js, $email, $row) {
  //TODO: change $email to pass actual email address
  
  //TODO: check to see if team member has a response to survey data and return
  // and error if he does.
  
  // delete an email
  $node = node_load($nid);
  foreach($node->team_email['und'] as $delta => $email_array) {
    if($email_array['safe_value'] == $email) {
      unset($node->team_email['und'][$delta]);
    }
  }
  node_save($node);
  
  if($js) {
    $row_to_delete = '#row' . $row;
    $commands = array();
    $commands[] = ajax_command_remove($row_to_delete);
    $commands[] = ajax_command_remove('.messages.error');
    $commands[] = ajax_command_restripe('#team-email-table');
      
    $page = array('#type' => 'ajax', '#commands' => $commands);
    ajax_deliver($page);
    return;
  }
  else {
    $output = t("Put a confirmation page here.");
    return $output;;
  }
  
}

function team_ajax_text_button($text, $dest, $alt, $class = '', $type = 'use-ajax') {
  drupal_add_js('misc/ajax.js');
  return l($text, $dest, array('html' => TRUE, 'attributes' => array('class' => array($type, $class), 'title' => $alt)));
}
