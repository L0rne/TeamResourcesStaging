<?php

/**
 * @file
 * Provides a wizard-style form to create new teams.
 */

/**
 * Return the list of steps and their associated forms. This has been separated
 * to clarify and easy the understanding of this example. You should edit this
 * function to include the steps your wizard/multistep form requires.
 *
 * @return $array
 */
function _team_wizard_steps() {
  return array(
      1 => array(
        'form' => 'team_wizard_name_and_description',
        //'title' => 'Welcome to the Benchmark Team Builder',
        'title' => 'Team Wizard: ',
        'next_action' => 'Add your team members',
      ),
      2 => array(
        'form' => 'team_wizard_distribution_list',
        //'title' => 'Team-Building 101: Add Team Members',
        'title' => 'Team Wizard: ',
        'next_action' => 'Apply your Diagnostic',
      ),
      3 => array(
        'form' => 'team_wizard_apply_purchases',
        //'title' => 'Select which diagnostic you would like to activate.',
        'title' => 'Team Wizard: ',
        'next_action' => 'Set a deadline',
      ),
      4 => array(
        'form' => 'team_wizard_set_deadline',
        //'title' => 'Set a due date to complete the diagnostic.',
        'title' => 'Team Wizard: ',
        'next_action' => 'Send invitations.',
      ),
      5 => array(
        'form' => 'team_wizard_send_invitation',
        //'title' => 'Review and customize team member survey notifications.',
        'title' => 'Team Wizard: ',
        'next_action' => 'Send this invitation and activate the survey',
      ),
    );
}

/**
 * helper method to get team_wizard_apply_purchases form index if this code changes.
 * 
 */
function _get_apply_step() {
  $steps = _team_wizard_steps();
  $step_number = -1;
  foreach ($steps as $delta => $step_info) {
    if ($step_info['form'] == 'team_wizard_apply_purchases') {
      $step_number = $delta;
    }
  }
  return $step_number;
}

/**
 * The primary formbuilder function for the wizard form. This is the form that
 * you should call with drupal_get_form() from your code, and it will include
 * the rest of the step forms defined. This form his will handle all the step
 * actions for you.
 *
 * This form has two defined submit handlers to process the different steps:
 *  - Previous: handles the way to get back one step in the wizard.
 *  - Next:     handles each step form submission,
 *
 * The third handler, the finish button handler, is the default form _submit
 * handler used to process the information.
 *
 */
function team_wizard($form, &$form_state) {
  
  // Initialize a description of the steps for the wizard.
  if (empty($form_state['step'])) {
    $form_state['step'] = 1;

    // This array contains the function to be called at each step to get the
    // relevant form elements. It will also store state information for each
    // step.
    $form_state['step_information'] = _team_wizard_steps();
    $purchases = get_available_diagnostics();
    if (count($purchases) < 1) {
      $form['no_purchase'] = array(
        '#type' => 'markup',
        '#markup' => '<div class="no-purchase">Please visit our <strong><a href="/diagnostic-products">Diagnostics</a></strong> pages and purchase a diagnostic before building a team</div>',
      );
      return $form;
    }
    $form_state['available_purchases'] = $purchases;
    
  }
  
  $step = &$form_state['step'];
  //drupal_set_title(t('Create Your Team: Step @step', array('@step' => $step)));
  drupal_set_title($form_state['step_information'][$step]['title']);

  // Call the function named in $form_state['step_information'] to get the
  // form elements to display for this step.
  $form = $form_state['step_information'][$step]['form']($form, $form_state);
  $form['#prefix'] = '<div id="team-create-wizard-step-' . $step. '">';
  $form['#suffix'] = '</div>';
  
  $step_legend_url = _get_step_image($form_state['step_information'][$step]['form']);
  $legend_vars = array(
    'path' => $step_legend_url,
    'width' => '940px',
    'height' => '70px',
    'alt' => 'Step ' . $step . ' of ' . count($form_state['step_information']),
    'title' => 'Step ' . $step . ' of ' . count($form_state['step_information']),
    'attributes' => array(),
  );
  $form['legend_image'] = array(
    '#type' => 'markup',
    '#weight' => -5,
    '#markup' => theme_image($legend_vars),
    '#prefix' => '<div class="wizard-legend">',
    '#suffix' => '</div>',
  );
  
  $directions_node = _get_step_details_node($form_state['step_information'][$step]['form']);
  $step_class = preg_replace('/_/', '-', $form_state['step_information'][$step]['form']);
  if($directions_node) {
    $markup = $directions_node->body[$directions_node->language][0]['value'];
    $form['email_directions'] = array(
      '#markup' => $markup,
      '#weight' => -1,
      '#prefix' => '<div class="wizard-directions ' . $step_class . '">',
      '#suffix' => '</div>',
    );
  }


  $form['navigation'] = array(
    '#tree' => TRUE,
    '#prefix' => '<div class="navigation-buttons">',
    '#suffix' => '</div>'
  );
  
  // Show the 'previous' button if appropriate. Note that #submit is set to
  // a special submit handler, and that we use #limit_validation_errors to
  // skip all complaints about validation when using the back button. The
  // values entered will be discarded, but they will not be validated, which
  // would be annoying in a "back" button.
  if ($step > 1) {
    $form['navigation']['prev'] = array(
      '#type' => 'submit',
      '#value' => t('Previous'),
      '#name' => 'prev',
      '#submit' => array('team_wizard_previous_submit'),
      '#attributes' => array('class' => array('prev-' . $step_class)),
      '#prefix' => '<div class="prev-button">',
      '#suffix' => '</div>',
      //'#limit_validation_errors' => array(),
    );
  }
  
  
  $next_step = $form_state['step_information'][$step]['next_action'];
  // Show the Next button only if there are more steps defined.
  if ($step < count($form_state['step_information'])) {
    // The Next button should be included on every step
    $form['navigation']['next'] = array(
      '#type' => 'submit',
      '#value' => $next_step,
      '#name' => 'next',
      '#submit' => array('team_wizard_next_submit'),
      '#attributes' => array('class' => array('next-' . $step_class)),
      '#prefix' => '<div class="next-button">',
      '#suffix' => '</div>',
    );
  }
  else {
    // Just in case there are no more steps, we use the default submit handler of
    // the form wizard. Call this button Finish, Submit, or whatever you want to
    // show. When this button is clicked, the team_wizard_submit handler
    // will be called.
    $form['navigation']['finish'] = array(
      '#type' => 'submit',
      '#value' => $next_step,
      '#prefix' => '<div class="finish-button">',
      '#suffix' => '</div>',
    );
  }

  // Include each validation function defined for the different steps.
  if (function_exists($form_state['step_information'][$step]['form'] . '_validate')) {
    $form['navigation']['next']['#validate'] = array($form_state['step_information'][$step]['form'] . '_validate');
  }
  
  // Include an additional prev submit function to handle step-specific logic.
  if (function_exists($form_state['step_information'][$step]['form'] . '_submit_prev')) {
    $form['navigation']['prev']['#submit'][] = $form_state['step_information'][$step]['form'] . '_submit_prev';
  }
  
  // Include an additional next submit function to handle step-specific logic.
  if (function_exists($form_state['step_information'][$step]['form'] . '_submit_next')) {
    $form['navigation']['next']['#submit'][] = $form_state['step_information'][$step]['form'] . '_submit_next';
  }

  return $form;
}

/**
 * Submit handler for the "previous" button.
 * - Stores away $form_state['values']
 * - Decrements the step counter
 * - Replaces $form_state['values'] with the values from the previous state.
 * - Forces form rebuild.
 *
 * You are not required to change this function.
 */
function team_wizard_previous_submit($form, &$form_state) {
  $current_step = &$form_state['step'];
  $form_state['step_information'][$current_step]['stored_values'] = $form_state['values'];
  if ($current_step > 1) {
    $current_step--;
    $form_state['values'] = $form_state['step_information'][$current_step]['stored_values'];
  }
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit handler for the 'next' button.
 * - Saves away $form_state['values']
 * - Increments the step count.
 * - Replace $form_state['values'] from the last time we were at this page
 *   or with array() if we haven't been here before.
 * - Force form rebuild.
 *
 * You are not required to change this function.
 *
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function team_wizard_next_submit($form, &$form_state) {
  $current_step = &$form_state['step'];
  $form_state['step_information'][$current_step]['stored_values'] = $form_state['values'];

  if ($current_step < count($form_state['step_information'])) {
    $current_step++;
    if (!empty($form_state['step_information'][$current_step]['stored_values'])) {
      $form_state['values'] = $form_state['step_information'][$current_step]['stored_values'];
    }
    else {
      $form_state['values'] = array();
    }
    $form_state['rebuild'] = TRUE;  // Force rebuild with next step.
    return;
  }
}

// The previous code was a 'skeleton' of a multistep wizard form. You are not
// required to change a line on the previous code (apart from defining your own
// steps in the _team_steps() function.
//
// All the code included from here is the content of the wizard, the steps of
// the form.

// First, let's show the defined steps for the wizard example.

/**
 * Returns form elements for the 'personal info' page of the wizard. This is the
 * first step of the wizard, asking for two textfields: first name and last
 * name.
 */
function team_wizard_name_and_description($form, &$form_state) {
  $form = array();
  $form['team_name'] = array(
    '#type' => 'textfield',
    '#title' => t('First, name your team'),
    '#required' => TRUE,
    '#default_value' => !empty($form_state['values']['team_name']) ? $form_state['values']['team_name'] : '',
  );
  $form['team_description'] = array(
    '#type' => 'textarea',
    '#cols' => 30,
    '#title' => t('Add a team description. (Optional)'),
    '#default_value' => !empty($form_state['values']['team_description']) ? $form_state['values']['team_description'] : '',
  );
  return $form;
}

/**
 * Returns form elements for the 'location info' page of the wizard. This is the
 * second step of the wizard. This step asks for a textfield value: a City. This
 * step also includes a validation declared later.
 */
function team_wizard_distribution_list($form, &$form_state) {
  $form = array();
  
  $form['email_list'] = array(
    '#type' => 'textarea',
    '#cols' => 30,
    '#title' => t('Team Email Distribution'),
    '#attributes' => array(
        'title' => t('Enter one address or multiple email addresses above. Enter one email address per line.'),
        'class' => array('textarea-hint'),
      ),
    '#required' => TRUE,
    '#default_value' => !empty($form_state['values']['email_list']) ? $form_state['values']['email_list'] : '',

  );
  return $form;
}

/**
 * Custom validation form for the 'location info' page of the wizard. This is the
 * validation function for the second step of the wizard. The city cannot be empty
 * or be "San Francisco".
 */
function team_wizard_distribution_list_validate($form, &$form_state) {

  $email_list = preg_split("/\\r\\n/", $form_state['values']['email_list']);
  foreach($email_list as $delta => $email) {
    if (!valid_email_address($email)) {
      form_set_error('email_err' . $delta, $email . ' is not a valid email address.');
    }
  }
}

function team_wizard_distribution_list_submit_next($form, &$form_state) {
  
  if(count($form_state['available_purchases']) > 1) {
    return;
  }
  
  $current_step = &$form_state['step'];
  $form_state['step_information'][$current_step]['stored_values'] = $form_state['values'];
  
  $line_ids = array_keys($form_state['available_purchases']);
  $form_state['step_information'][_get_apply_step()]['stored_values']['line_items'][$line_ids[0]] = $line_ids[0];
  
  if ($current_step < count($form_state['step_information'])) {
    $current_step++;
    if (!empty($form_state['step_information'][$current_step]['stored_values'])) {
      $form_state['values'] = $form_state['step_information'][$current_step]['stored_values'];
      //$form_state['step_information'][$current_step]['stored_values']['line_items'] = $form_state['available_purchases'];
    }
    else {
      $form_state['values'] = array();
    }

    return;
  }
  
}

/**
 * Returns form elements for the 'apply info' page of the wizard. This is the
 * thid and last step of the example wizard.
 */
function team_wizard_apply_purchases($form, &$form_state) {
  $form = array();
  
  $purchases = $form_state['available_purchases'];
  $applied = array();
  $apply_step = _get_apply_step();
  if (isset($form_state['step_information'][$apply_step]['stored_values']['line_items'])) {
    $applied = $form_state['step_information'][$apply_step]['stored_values']['line_items'];
  }
  
  $form['applied_purchases']['line_items'] = array(
    '#type' => 'checkboxes',
    '#options' => $purchases, // drupal_map_assoc($purchases),
    '#default_value' => $applied,
    '#required' => FALSE,
  );
  
  return $form;
}

function team_wizard_apply_purchases_validate($form, &$form_state) {
  
  $none_selected = true;
  $line_items = $form_state['values']['line_items'];
  foreach ($line_items as $line_id) {
    if ($line_id > 0) {
      $none_selected = false;
    }
  }
  
  if ($none_selected) {
    form_set_error('at_least_one', 'Select at least one diagnostic.');
  }
}

function team_wizard_set_deadline($form, &$form_state) {
  
  $applied = $form_state['step_information'][_get_apply_step()]['stored_values']['line_items'];
  $purchases = $form_state['available_purchases'];
  
  $date = date("Y-m-d", strtotime("+2 weeks"));// 2 weeks from today
  
  foreach ($applied as $line_id) {
    if($line_id > 0) {
      $diagnostic = $purchases[$line_id];
      $form['deadlines'][$line_id] = array(
        '#type' => 'date_popup',
        '#title' => 'Set due date for ' . $diagnostic,
        '#date_format' => 'Y-m-d',
        '#default_value' => $date,
        '#date_label_position' => 'none',
        '#date_year_range' => '0:+1',
        '#description' => '',
        '#date_text_parts' => array('year','month','day'),
      );
    }
  }
  
  return $form;
}

function team_wizard_set_deadline_submit_prev($form, &$form_state) {
  
  if (count($form_state['available_purchases']) < 2) {
    $current_step = &$form_state['step'];
    $form_state['step_information'][$current_step]['stored_values'] = $form_state['values'];
    if ($current_step > 1) {
      $current_step--;
      $form_state['values'] = $form_state['step_information'][$current_step]['stored_values'];
    }
  }
}

/**
 * Returns form elements for the 'send info' page of the wizard. This is the
 * thid and last step of the example wizard.
 */
function team_wizard_send_invitation($form, &$form_state) {
  $form = array();
  $form['invitation_message'] = array(
    '#type' => 'textarea',
    '#cols' => 30,
    '#title' => t("Invitation Message"),
    '#attributes' => array(
        'title' => t('Send your team an invitation. We will append team member instructions and survey links to the end of each email message.'),
        'class' => array('textarea-hint'),
      ),
    '#default_value' => !empty($form_state['values']['invitation_message']) ? $form_state['values']['invitation_message'] : '',
  );
  
  $boilerplate_alias = 'includes/survey-participant-invitation';
  $boilerplate_path = explode('/', drupal_get_normal_path($boilerplate_alias));
  $boilerplate_id = $boilerplate_path[1];
  $boilerplate = node_load($boilerplate_id);
  
  $form['boilerplate'] = array(
    '#type' => 'markup',
    '#markup' => $boilerplate->body['und'][0]['value'],
    '#prefix' => '<div class="boilerplate-message">',
    '#suffix' => '</div>',
  );
  
  $form_state['boilerplate'] = $boilerplate->body['und'][0]['value']; 
  // The Send button
  /*
    $form['send'] = array(
      '#type' => 'button',
      '#value' => t('Send'),
      '#name' => 'send',
      '#submit' => array('team_wizard_send_invitation'),
      '#suffix' => '<br /><br />',
    );
  */  
  return $form;
}


// And now comes the magic of the wizard, the function that should handle all the
// inputs from the user on each different step.
/**
 * Wizard form submit handler.
 * - Saves away $form_state['values']
 * - Process all the form values.
 *
 * This demonstration handler just do a drupal_set_message() with the information
 * collected on each different step of the wizard.
 *
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function team_wizard_submit($form, &$form_state) {
  
  $current_step = &$form_state['step'];
  $form_state['step_information'][$current_step]['stored_values'] = $form_state['values'];
  
  // Process the submitted information.
  
  //drupal_set_message(t('This information was collected by this wizard:'));
  $form_values = array();
  foreach ($form_state['step_information'] as $index => $value) {
    // Remove FAPI fields included in the values (form_token, form_id and form_build_id
    // This is not required, you may access the values using $value['stored_values']
    // but I'm removing them to make a more clear representation of the collected
    // information as the complete array will be passed through drupal_set_message().
    unset($value['stored_values']['form_id']);
    unset($value['stored_values']['form_build_id']);
    unset($value['stored_values']['form_token']);
    unset($value['stored_values']['next']);
    unset($value['stored_values']['prev']);
    unset($value['stored_values']['send']);
    unset($value['stored_values']['finish']);
    unset($value['stored_values']['op']);
    

    // Now show all the values.
    //drupal_set_message(t('Step @num collected the following values: <pre>@result</pre>', array('@num' => $index, '@result' => print_r($value['stored_values'], TRUE))));
    $form_values = array_merge($form_values, $value['stored_values']);
    
  }
  
  $form_values['nid'] = _create_team_node($form_values);
  $form_values['boilerplate'] = $form_state['boilerplate'];
  _send_invitations($form_values);
  
  $form_state['redirect'] = 'node/4';
  return;
  
}

function _create_team_node($form_values) {
  
  $new_team = new stdClass();
  $new_team->type = "team";
  node_object_prepare($new_team);
  
  $new_team->title = $form_values['team_name'];
  $new_team->language = LANGUAGE_NONE;
  
  $body_text = $form_values['team_description'];;
  $new_team->body[$new_team->language][0]['value']   = $body_text;
  $new_team->body[$new_team->language][0]['summary'] = text_summary($body_text);
  $new_team->body[$new_team->language][0]['format']  = 'plain_text';
  //$new_team->comment = '0';

  // add email field
  $email_list = preg_split("/\\r\\n/", $form_values['email_list']);
  $unique_emails = array_unique($email_list);
  $team_email_array = array();
  foreach($unique_emails as $delta => $email) {
    $team_email_array = array(
      'value' =>  $email, //$form_state['values']['new_email'],
      'format' => null,
      'safe_value' => $email,
    );
    $new_team->team_email[$new_team->language][] = $team_email_array;
  }
  
  // add applied diagnostic field
  foreach ($form_values['line_items'] as $line_item_id) {
    if($line_item_id > 0) { 
      $new_team->diagnostic_line_item_id[$new_team->language][]['value'] = $line_item_id;
    }
  }
  
  node_save($new_team);
  return $new_team->nid;
}

function _send_invitations($form_values) {
  
  global $user;
  global $base_url;
  
  $from = $user->mail;
  $subject = t('Message from !team', array('!team' => $user->name));
  
  $email_list = preg_split("/\\r\\n/", $form_values['email_list']);
  $unique_emails = array_unique($email_list);
  $nid = $form_values['nid'];
  
  $output = '';
  $msg_count = 0;
  $msg_type = 'status';
  foreach($unique_emails as $email) {
    $to = $email;
    // build survey links and send
    foreach ($form_values['line_items'] as $line_item_id) {
    if($line_item_id > 0) {
        $msg = $form_values['invitation_message'] . '<br /><br />' . $form_values['boilerplate'];
        $link = $base_url . '/team-diagnostic/survey/' . $nid . '/' . $line_item_id . '/' . $to;
        if (!send_team_email($to, $from, $subject, $msg, $link)) {
          $msg_type = 'error';
          $output .= "Error sending to $to<br />";
          //$output .= $link . '<br />';
        }
        else {
          $msg_count++;
          
        }
      }
    }

    
  }
  $message_sent_text = $msg_count == 1 ? '1 Message' : $msg_count . ' Messages';
  $output = "<h3>$message_sent_text Sent!</h3>" . $output;
  $message_clarification = t('If you have more than one diagnostic applied to this team, a separate email was sent to each member for each diagnostic.');
  $output .= "<br>($message_clarification)";
  drupal_set_message($output, $msg_type); // t("Email messages sent. Instructions and links were appended to your message.")); //
  
}

/**
 * Helper function to get step directions from a node.
 *
 */
function _get_step_details_node($step_name) {
  $node_url_alias = 'includes/' . preg_replace('/_/', '-', $step_name);
  $directions_path = explode('/', drupal_get_normal_path($node_url_alias));
  $directions_id = $directions_path[1];
  $directions_node = node_load($directions_id);

  return $directions_node;
}

function _get_step_image($step_name) {
  $image_filename = preg_replace('/_/', '-', $step_name) . '.png';
  $image_url = file_create_url('public://assets/' . $image_filename);
  
  return $image_url;
}