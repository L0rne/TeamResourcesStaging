<?php
// $Id$

/**
 * @file
 * Implements team diagnostic survey functionality
 *
 * TODO: Refactor the Diagnostic data implementation. Instead of a custom table
 * with the Diagnostic statements, use a Diagnostic content type. The survey
 * items can be a content type associated with the diagnostic via a node
 * reference. A node_reference can associate the diagnostic to its product entry
 * and survey items.
 * 
 */

define('ITEM_PREFIX', 'diagnostic_item_');
define('COMMENT_PREFIX', 'comment_');
require_once(dirname(__FILE__) . '/team_diagnostics.data-access.inc');

function team_diagnostic_survey_form($form, &$form_state) {
  //require_once(dirname(__FILE__) . '/team_diagnostics.data-access.inc');
  
  if(empty($form_state['response_id'])) {
    // pull arguments from URL
    $args = $form_state['build_info']['args'];
    
    // make sure there are 3 arguments
    if(count($args) != 3) {
      drupal_set_message('You do not have the proper access information. Please check your URL.', 'error');
      return array();
    }
    
    // get a response_id from the database if arguments are valid, otherwise return an error message.
    try {
      $form_state['response_id'] = get_response_id($args);
    }
    catch(Exception $e) {
      drupal_set_message($e->getMessage(), 'error');
      return array();
    }
    
  }
  
  $response_id = $form_state['response_id'];
  
  $form['#prefix'] = '<div id="survey-form-wrapper">';
  $form['#suffix'] = '</div>';
  
  if (empty($form_state['page'])) {
    $form_state['page'] = 0;
  }
  
  $diagnostic_id = -1;
  if (empty($form_state['diagnostic_id'])) {
    $diagnostic_id = diagnostic_id_from_line_item($args[1]);
    $form_state['diagnostic_id'] = $diagnostic_id;
  }
  else {
    $diagnostic_id = $form_state['diagnostic_id'];
  }
  
  if (empty($form_state['page_max'])) {
    
    $form_state['page_max'] = get_last_page_number($diagnostic_id);
  }
  
  
  $page = $form_state['page'];
  $max_page = $form_state['page_max'];
  
  // TODO: refactor this to fetch multiple items
  $item_info = array(
    'diagnostic_id' => $diagnostic_id,
    'page' => $page,
    'response_id' => $response_id,
  );
  $survey_item = get_diagnostic_survey_item($item_info);
  $form_state['item_info'] = $survey_item;
  
  $item_name = ITEM_PREFIX . $survey_item['item_id'];
  $form[$item_name] = array(
    '#title' => $survey_item['text'],
    '#type' => 'survey_form_radio_scale', 
    '#options' => $survey_item['options'],
    '#default_value' => $survey_item['response_value'],
    '#prefix' => '<div id="answer-options">',
    '#suffix' => '</div>',
    //#required' => TRUE,
  );
  
  $comment_field_name = COMMENT_PREFIX . $survey_item['item_id'];

  $comment_field_title = "Comments";
  $comment_field_desc = "Add any feedback you think is relevant to this statement (Optional)";
  $statement = "Statement";
  $of = "of";
  $previous = 'Previous';
  $next = "Next";
  $finish = "Finish";

  // Check language
  global $language;
  $lang = $language->language;
  if ($lang == "es") {
    $comment_field_title = "Comentarios";
    $comment_field_desc = "Agregue cualquier comentario que considere relevante para esta afirmación (Opcional)";
    $statement = "Afirmación";
    $of = "de";
    $previous = 'Anterior';
    $next = "Siguiente";
    $finish = "Terminar";
  }

  $form[$comment_field_name] = array(
    '#prefix' => '<div id="comment-feedback">',
    '#suffix' => '</div>',
    '#title' => $comment_field_title,
    '#type' => 'textarea',
    '#description' => $comment_field_desc,
    '#default_value' => $survey_item['comments'],
    '#cols' => 40,
    '#rows' => 3,
    '#resizable' => TRUE,
  );
  
  $pager_label = t($statement . ' !current ' . $of . ' !max', array('!current' => ($page + 1), '!max' => ($max_page + 1)));
  $form['page_number'] = array(
    '#markup' => '<h2 id="diagnostic-survey-page">' . $pager_label . '</h2>',
  );
  
  if($page > 0) {
    $form['prev'] = array(
      '#type' => 'submit',
      '#attributes' => array('class' => array('survey-prev-button')),
      '#value' => $previous,
      '#submit' => array('team_diagnostic_survey_form_prev_submit'),
      '#ajax' => array(
          'wrapper' => 'survey-form-wrapper',
          'callback' => 'survey_form_ajax_callback',
          'effect' => 'fade',
          'speed' => 'slow',
      ),
    );
  }
  
  $next_label = $page < $form_state['page_max'] ? $next : $finish;
  $form['next'] = array(
    '#type' => 'submit',
    '#attributes' => array('class' => array('survey-next-button')),
    '#value' => $next_label,
    '#submit' => array('team_diagnostic_survey_form_next_submit'),
    '#ajax' => array(
        'wrapper' => 'survey-form-wrapper',
        'callback' => 'survey_form_ajax_callback',
        'effect' => 'fade',
        'speed' => 'slow',
    ),
  );
  
  // on last page submit "Finish", we do not want an AJAX effect (plus, it complicates things)
  if($page > ($form_state['page_max'] - 1)) 
  {
    unset($form['next']['#ajax']);
  }
  
  return $form;
    
}

function team_diagnostic_survey_form_validate($form, &$form_state) {
  $form_values = $form_state['values'];
  $trigger_button = $form_state['triggering_element']['#value'];
  $data_item_id = 'diagnostic_item_' . $form_state['item_info']['item_id'];
  $user_answer = $form_values[$data_item_id];
  $previous = FALSE;
  if ($trigger_button == 'Previous' || $trigger_button == 'Anterior') {
    $previous = TRUE;
  }

  $no_answer_text = 'You must select an answer to move to the next question.';

  // Check language
  global $language;
  $lang = $language->language;
  if ($lang == "es") {
    $no_answer_text = "Debe seleccionar una respuesta para pasar a la siguiente pregunta.";
  }

  if ($user_answer == 0 && !$previous) {
    form_set_error('no_answer', $no_answer_text);
  }
}

function team_diagnostic_survey_form_next_submit($form, &$form_state) {
  //require_once(dirname(__FILE__) . '/team_diagnostics.data-access.inc');
  
  $current_page = &$form_state['page'];
  
  // gather response data and save
  $item_id = $form_state['item_info']['item_id'];
  $item_label = ITEM_PREFIX . $item_id;
  $comment_label = COMMENT_PREFIX . $item_id;
  $user_response = array(
    'response_id' => $form_state['response_id'],
    'item_id' => $item_id, //$current_page,
    'item_points' => $form_state['values'][$item_label],
    'comments' => $form_state['values'][$comment_label],
  );
  save_response_detail($user_response);
  
  // go to next survey item or redirect to thank you page.
  if($current_page > ($form_state['page_max'] - 1)) {
    // save and redirect
    complete_response($form_state['response_id']);
    $form_state['redirect'] = 'node/4';
  }
  else {
    $form_state['page']++;
    $form_state['rebuild'] = TRUE;
  }
}

function team_diagnostic_survey_form_prev_submit($form, &$form_state) {
  //require_once(dirname(__FILE__) . '/team_diagnostics.data-access.inc');
  $current_page = &$form_state['page'];
  
    // gather response data and save
  $item_id = $form_state['item_info']['item_id'];
  $item_label = ITEM_PREFIX . $item_id;
  $comment_label = COMMENT_PREFIX . $item_id;
  $user_response = array(
    'response_id' => $form_state['response_id'],
    'item_id' => $item_id, //$current_page,
    'item_points' => $form_state['values'][$item_label],
    'comments' => $form_state['values'][$comment_label],
  );
  if ($user_response['item_points'] > 0) {
    save_response_detail($user_response);
  }
  
  if($current_page > 0) {
    $form_state['page']--;
    $form_state['rebuild'] = TRUE;
  }
}

function survey_form_ajax_callback ($form, &$form_state) {
    return $form;
}

function survey_title_callback() {
    // ToDo: Fetch Diagnostic and Team IDs from URL placeholders and generate a dynamic title.
    $args = preg_split('/\//', $_GET['q']);
    if (count($args) != 5) {
      return t('Access Denied');
    }
    $team_node_id = $args[2];
    $diagnostic_line_item_id = $args[3];
    
    $title_array = get_team_diagnostic_info_from_line_item($diagnostic_line_item_id);
    
    $title = $title_array['team_name'] . ': ' . $title_array['diagnostic_name'] . " questionnaire";

    // Check language
    global $language;
    $lang = $language->language;
    if ($lang == "es") {
      $title = $title_array['team_name'] . ': Cuestionario de ' . $title_array['diagnostic_name'];
    }


    return $title;
}

/**
 *  Custom form element: Radio Scale Item
 */

function theme_survey_form_radio_scale($variables) {
  $element = $variables['element'];
  $children = element_children($element);
  $output = '';
  $headers = array();
  $cells = array();
  foreach($children as $key => $value) {
    $headers[] = '<label for="' . $element[$value]['#id'] . '">' . $element[$value]['#title'] . '</label>';
    $cells[$key]['data'] = theme('radio', $element[$value]);
    $cells[$key]['align'] = 'center';
  }
  $rows = array(
        'data' => $cells,
  );
  
  $tablevars = array(
        'header' => $headers,
        'rows' => $rows,
        'attributes' => array(),
        'caption' => NULL,
        'colgroups' => NULL,
        'sticky' => 0,
        'empty' => NULL,
    );
  
  $output = theme_table($tablevars);

  return $output;
}
