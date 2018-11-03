<?php
// $Id$

/**
 * @file
 * Implements functionality for team node content type
 *
 * 
 */

/*******************************************************************************
 *  Implements the Team Content Type
 ******************************************************************************/

/**
 * Implements hook_node_info() to provide our team type.
 * (see Pro D7 Dev p. 142)
 */
function team_diagnostics_node_info() {
  return array(
    'team' => array(
      'name' => t('Team'),
      'base' => 'team',
      'description' => t('Use this content type to create teams.'),
      'has_title' => TRUE,
      'title_label' => t('Team Name'),
      'help' => t('Enter the team name and description'),
    ),
  );
}

/**
 * Implement hook_form() with the standard default form.
 * (see Pro D7 Dev p. 148)
 */
function team_form($node, $form_state) {
  return node_content_form($node, $form_state);
}

/**
 * Implementation of hook_permission().
 */
function team_permission() { 
  return array(
  'create team' => array(
    'title' => t('Create a team'),
    'description' => t('Create a team'),
  ),
  'edit own team' => array(
    'title' => t('Edit own team'),
    'description' => t('Edit your own team'),
  ),
  'edit any team' => array(
    'title' => t('Edit any team'),
    'description' => t('Edit any team'),
  ),
  'delete own team' => array(
    'title' => t('Delete own team'),
    'description' => t('Delete own team'),
  ),
  'delete any team' => array(
    'title' => t('Delete any team'),
    'description' => t('Delete any team'),
  ),
 );
}

/**
 * Implements hook_node_access().
 *
 * For more info on how node access rights work, see:
 * http://api.drupal.org/api/drupal/modules--node--node.module/group/node_access/7
 *
 * For more info on this hook, see:
 * http://api.drupal.org/api/drupal/modules--node--node.api.php/function/hook_node_access/7
 *
 * Note that if we return NODE_ACCESS_ALLOW or NODE_ACCESS_DENY, that will
 * override any other access settings set by other modules.
 */
function team_diagnostics_node_access($node, $op, $account) {

  global $user;
  
  if(in_array('administrator', array_values($user->roles))) {
    return NODE_ACCESS_IGNORE;
  }
  
  if (($op == 'view') && (isset($account->uid) && $account->uid != $node->uid) && (isset($node->type) && $node->type == 'team')) {
    drupal_set_message(t('You are attempting to access a team you do not manage. Please check your URL.'), 'error');
    return NODE_ACCESS_DENY;
  }
  
  return NODE_ACCESS_IGNORE;
}

/**
 * Implementation of hook_load().
 */
function team_load($nodes) { 
// Add a new element to the node at load time for storing the team code information
  foreach ($nodes as $node) {
    $node->team_code = md5($node->nid); //clean_team_title($node->title) . '-' . dechex($node->nid); //md5($node->nid);
  }

  return $node;
}


/**
 * Implement hook_view().
 */
 function team_view($node, $view_mode) {
  
  if ($view_mode == 'full') {
    
    $jquery = libraries_get_path('jquery.jeditable') . '/jquery.jeditable.js';
    $js = drupal_get_path('module', 'team_diagnostics') . '/drupal_jeditable.js';
    $css = drupal_get_path('module', 'team_diagnostics') . '/team.css';
    $css2 = drupal_get_path('theme', 'omega_benchmark') . '/css/team-page.css';
    drupal_add_js($jquery);
    drupal_add_js($js);
    drupal_add_css($css);
    drupal_add_css($css2, array('group' => CSS_THEME, 'weight' => 100));
    
    
    //$nid = $node->nid;
    //$body = $node->body['und'][0]['safe_value'];
    //$new_body = '<div id="team-node-body-' . $nid . '"  class="team-node-body jeditable-textarea">' . $body . '</div>';
    //$new_title = '<span id="team-node-title-' . $nid . '"  class="team-node-title jeditable-textfield">' . $node->title . '</span>';
    
    //$node->title = $new_title; // NOTE: This setting may need reinforcement in the theme's template.php
    //$node->body['und'][0]['safe_value'] = $new_body;
    //$node->body['und'][0]['value'] = $new_body;
      
    //drupal_add_js('misc/ajax.js');
    
    //$node->content['add_email_form'] = drupal_get_form('add_team_email_form', $node);
    //$node->content['add_email_form']['#weight'] = 110;
    
    //$node->content['team_code'] = array(
    //  '#markup' => '<div class="team-code-block"><p><em>' . t('Team Code: !tcode', array('!tcode' => $node->team_code)) . '</em></p></div>',
    //  '#weight' => 200,
    //);
  }
  return $node; 
 }
 
 /*
 function team_diagnostics_node_load($nodes, $types) {
  
  foreach ($nodes as $node) {
    if ($node->type == 'team') {
      $nid = $node->nid;
      $body = $node->body['und'][0]['safe_value'];
      $new_body = '<div id="node-body-' . $nid . '">' . $body . '</div>';
      
      //$node->body['und'][0]['safe_value'] = $new_body;
      //$node->body['und'][0]['value'] = $new_body;
    }
  }
  return;
}
 */
 
 /*
 function add_team_email_form($form_state, $node) {
  //$form['action']['submit']['#ajax']
  $node_base = $node['build_info']['args'][0];
  
  $form['#prefix'] = '<div id="team-email-distribution-form">';
  $form['#suffix'] = '</div>';
  
  $list_output = '<div class="team-instruction">TODO: Add team member emails using the form below:</div>';
  if(array_key_exists('und', $node_base->team_email)) {
    $list_output = get_team_distribution_widget($node_base->nid, $node_base->team_email['und']);
  }
  $form['email_list'] = array(
    '#markup' => $list_output,
    '#prefix' => '<div id="team-email-distribution-list">',
    '#suffix' => '</div>',
  );
  
  $form['user_message'] = array(
    '#markup' => '<div style="display:none"></div>',
  );
  
  $form['new_email'] = array(
    '#title' => 'Email',
    '#type' => 'textarea',
    '#description' => 'Enter multiple email address. One email per line or separated by commas.',
    '#default_value' => '',
    '#prefix' => '<div id="new-email-field">',
    '#suffix' => '</div>',
    '#required' => TRUE,
    '#element_validate' => array('new_email_validate'),
  );
  
  // For convenience, save the node id.
  $form['nid'] = array( 
    '#type' => 'value', 
    '#value' => $node_base->nid,
  );

  $form['add_button'] = array(
    '#type' => 'button',
    '#ajax' => array(
      'callback' => 'ajax_add_team_email_callback',
      'wrapper' => array('team-email-distribution-form'), //, 'new-email-field'),
      'name' => 'add1',
    ),
    '#value' => t('Add Email to Team'),
  );
  
  return $form;
}

function get_team_distribution_widget($nodeid, $team_email_list) {
  
  //$headers = array('Email', 'Remove');
  $rows = array();
  
  foreach($team_email_list as $delta => $email_object) {
    $email = $email_object['safe_value'];
    $row = array(
      'id' => 'row' . $delta,
      'class' => array('team-email-row-' . $delta),
      'data' => array(
                $email,
                team_ajax_text_button(t('remove'), "team/$nodeid/nojs/delete/$email/$delta", t('Remove this team member')),
      ),
    );
      
    $rows[] = $row;
  }
  
  $header = array(t('Email'), t('Action'));
  
  $tablevars = array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'team-email-table'),
    'caption' => 'Team Distribution List',
    'colgroups' => NULL,
    'sticky' => 0,
    'empty' => NULL,
  );
  
  return theme_table($tablevars);
}

function new_email_validate($element, &$form_state) {
  
  $emailvalues = get_email_array($element['#value']);
  
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
  
  
  $output = get_team_distribution_widget($node->nid, $node->team_email['und']);
  
  $form['email_list']['#markup'] = $output;
  $form['new_email']['#value'] = '';
  $form['user_message']['#markup'] = '';
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
*/

/**
 * Check to see if the incoming menu item is js capable or not.
 */
function team_js_load($js) {
  if ($js == 'ajax') {
    return TRUE;
  }
  return 0;
}
