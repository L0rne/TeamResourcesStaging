<?php

/**
 * @file
 * This file is empty by default because the base theme chain (Alpha & Omega) provides
 * all the basic functionality. However, in case you wish to customize the output that Drupal
 * generates through Alpha & Omega this file is a good place to do so.
 * 
 * Alpha comes with a neat solution for keeping this file as clean as possible while the code
 * for your subtheme grows. Please read the README.txt in the /preprocess and /process subfolders
 * for more information on this topic.
 */

 /**
  * Implements hook_theme()
  */
function omega_benchmark_theme($existing, $type, $theme, $path) {
    $items = array();
    // create custom user-login.tpl.php
    $items['user_login'] = array(
        'render element' => 'form',
        'path' => drupal_get_path('theme', 'omega_benchmark') . '/templates',
        'template' => 'user-login',
        'preprocess functions' => array(
            'omega_benchmark_preprocess_user_login'
        ),
    );
    
    $items['user_register_form'] =array( //user_register_form
        'render element' => 'form',
        'path' => drupal_get_path('theme', 'omega_benchmark') . '/templates',
        'template' => 'user-register',
        'preprocess functions' => array(
            'omega_benchmark_preprocess_user_register'
        ),f
    );
    return $items;
}

function omega_benchmark_menu_link(&$vars) {
  $element = $vars['element'];
  $sub_menu = '';

  if ($element['#below']) {
    $sub_menu = drupal_render($element['#below']);
  }
  
  // core user page does not register in the menus as "active"
  // so force it to be active...
  if (arg(0) == 'user' && substr($element['#href'], 0, 4) == 'user') {
    $element['#attributes']['class'][] = 'active-trail';
    $element['#localized_options']['attributes']['class'][] = 'active';
  }
  
  // new menu_css_names code starts here
  //
  // add a class that is equal to the title of the menu link,
  // replacing special characters with dashes.
  if (!empty($element['#title'])) {
    $element['#attributes']['class'][] = _benchmark_make_class_name($element['#title']);
  }
  // end of menu_css_names code
  
  $output = l($element['#title'], $element['#href'], $element['#localized_options']);
  return '<li ' . drupal_attributes($element['#attributes']) . '>' . $output . $sub_menu . "</li>\n";
}

//
// helper function to build class names, with tags and other characters stripped out.
//
function _benchmark_make_class_name($text) {
	// do main text conversion to class name,
	// then remove double hyphens or hyphens at beginning or end of class name
	$text = drupal_strtolower(preg_replace('/(\s?&amp;\s?|[^-_\w\d])/i', '-', trim(strip_tags($text))));
	$text = preg_replace('/(^-+|-+$)/', '', $text);
	$text = preg_replace('/--+/', '-', $text);
	return $text;
}


function omega_benchmark_preprocess_page(&$vars) {
  //drupal_add_js(path_to_theme() . '/scripts/jquery.easing.js');
  //drupal_add_js(path_to_theme() . '/scripts/jTour.js');
  drupal_add_js(path_to_theme() . '/scripts/benchmark-theme.js');
  //drupal_add_css(path_to_theme() . '/css/skins/square/aero.css');  
  drupal_add_css(path_to_theme() . '/css/theme8/style.css');
  
  

  
 
  
  $ua = strtolower($_SERVER['HTTP_USER_AGENT']);
  if (preg_match('/msie/', $ua)) {
    drupal_add_css(path_to_theme() . '/css/iefix.css');
    //die(path_to_theme() . '/css/iefix.css');
  }
  
  $args = preg_split('/\//', $_GET['q']);
  
  global $user;
  $order_id = commerce_cart_order_id($user->uid);
  if ($order_id) {
    drupal_add_js(libraries_get_path('tabslideout') . '/jquery.tabSlideOut.v1.3.js');
    drupal_add_js(path_to_theme() . '/scripts/shopping-cart.js');
  }

  if (isset($vars['node']->type)) {
    if ($vars['node']->type == 'team') {
     // if (!isset($_COOKIE['firsttime'])) {
     //   setcookie("firsttime", "no" /* EXPIRE */);
     //   drupal_add_js(path_to_theme() . '/scripts/team-first-time.js');
     // }
      
      //TODO: Get Micro Copy (content type) nodes with Vocab Term "team" and build js in the format below...
     // $micro_query = new EntityFieldQuery();
     // $entities = $micro_query->entityCondition('entity_type', 'node')
     //                         ->entityCondition('bundle', 'micro_copy')
     //                         ->fieldCondition('field_for_pages', 'tid', '4', '=') // tid 4 = 'Team Admin'
     //                         ->execute();
      
     // $copy = node_load_multiple(array_keys($entities['node']));
     // $js = '';
     // foreach ($copy as $qtip) {
     //   $title = addslashes($qtip->title);
     //   $text = addslashes($qtip->body[$qtip->language][0]['value']);
     //   $js .= 'triaxia.team_admin.' . $qtip->field_variable_name[$qtip->language][0]['safe_value'] . ' = ';
     //   $js .= '{title: "' . $title . '", text: "' . $text . '"}; ';
     // }
     // drupal_add_js($js, 'inline');
     
     // //drupal_add_js('triaxia.team_admin.step_1 = {title: "Step One:", text: "Change your team name"};', 'inline');
      
      $vars['title'] = '<div id="team-node-title-' . $vars['node']->nid . '"  class="team-node-title jeditable-textfield">' .  $vars['node']->title . '</div>';
    }
  }
}

function omega_benchmark_preprocess_field(&$variables, $hook) {
  if ($variables['element']['#bundle'] != 'team' || $variables['element']['#view_mode'] != 'full') {
    return;
  }
  
  $field_name = $variables['element']['#field_name'];
  $nid = $variables['element']['#object']->nid;
  switch ($field_name) {
    case 'body':
      // make field label translatable
        $variables['label'] = t($variables['label']);
        $variables['classes_array'][] = 'team-description-field';
      $body = $variables['items'][0]['#markup'];
      $variables['items'][0]['#markup'] = '<div id="team-node-body-' . $nid . '"  class="team-node-body jeditable-textarea">' . $body . '</div>';
      break;
  }
  return;
}
/*
function omega_benchmark_field($variables) {
  return;
}
*/

function omega_benchmark_alpha_preprocess_region(&$vars) { 
  $menu_object = menu_get_object();
  if (isset($menu_object->type) && $vars['region'] == 'content') {
    if ('team' == $menu_object->type) {
      $vars['theme_hook_suggestions'][] = 'region__content_team';
    }
  }
}

function omega_benchmark_preprocess_user_login(&$variables) {
  //kpr($variables);
  $form = $variables['form'];
  $variables['username_field'] = drupal_render($form['name']);
  $variables['password_field'] = drupal_render($form['pass']);
  $variables['form_submit_action'] = drupal_render_children($form);
}

function omega_benchmark_preprocess_user_register(&$variables) {
    //kpr($variables);
  $form = $variables['form'];
  $variables['account_name'] = drupal_render($form['account']['name']);
  $variables['account_mail'] = drupal_render($form['account']['mail']);
  $variables['confirm_email'] = drupal_render($form['account']['mail_verify']);
  $variables['account_password'] = drupal_render($form['account']['pass']);
  
  $variables['form_submit_action'] = drupal_render_children($form);
}

/**
 *  Modifications to commerce pages and form elements
 *
 */

/** TODO: DELETE THIS BLOCK
function omega_benchmark_commerce_checkout_pages_alter($checkout_pages, $review) {
  return;
}

function omega_benchmark_commerce_checkout_pane_info_alter($checkout_pane) {
  return;
}

function omega_benchmark_form_alter(&$form, &$form_state, $form_id) {
  return;
}


function omega_benchmark_form_views_form_commerce_cart_form_default_alter(&$form, &$form_state, $form_id) {
  //$delete_buttons = $form['edit_delete'];
  foreach ($form['edit_delete'] as $delta => $delete_button) {
    if ($delta == '#tree') continue;
    $form['edit_delete'][$delta]['#limit_validation_errors'] = array();
  }
  
  $form_state['rebuild'] = TRUE;
  return;
}

function omega_benchmark_form_commerce_checkout_form_checkout_alter(&$form, &$form_state, $form_id) {
  return;
}
*/

/*
function omega_benchmark_form_alter(&$form, &$form_state, $form_id) {
  return;
}
*/

//function omega_benchmark_form_user_register_form_alter(&$form, &$form_state) {
//  //kpr($form_id);
//}

function omega_benchmark_form_user_profile_form_alter(&$form, &$form_state, $form_id) {
  global $user;
  $access_contact_form = in_array('administrator', $user->roles);
  $form['contact']['#access'] = $access_contact_form;
  $form['#submit'][] = 'omega_benchmark_user_profile_form_redirect';
  return;
}

function omega_benchmark_user_profile_form_redirect($form, &$form_state) {
  $form_state['redirect'] = 'user';
}

function omega_benchmark_add_continue_shopping_link(&$form, $weight = 100) {
  $options = array('title' => 'Continue Shopping', 'attributes' => array('class' => 'continue-shopping-link'));
  $form['continue-shopping'] = array(
    '#prefix' => '<div class="continue-shopping">',
    '#suffix' => '</div>',
    '#markup' => l(t('Continue Shopping'), 'benchmark-diagnostics', $options),
    '#weight' => $weight,
  );
  
}

function omega_benchmark_form_user_register_form_alter(&$form, &$form_state, $form_id) {
  
  //$form['account']['mail']['#weight'] = 0;
  $form['account']['mail_verify'] = array(
    '#type' => 'textfield',
    '#title' => t('Verify Email Address'),
    '#maxlength' => 254,
    '#description' => t('Re-type your email address.'),
    '#required' => 1,
    '#default_value' => '',
    '#size' => 30,
    //'#weight' => 0,
  );
  
  $form['#validate'][] = 'omega_benchmark_validate_user_registration';
  
}

function omega_benchmark_validate_user_registration(&$form, &$form_state) {
  
  if ($form_state['values']['mail'] != $form_state['values']['mail_verify']) {
    form_set_error('inconsistent_email', t('Your email addresses do not match'));
  }
}

function omega_benchmark_form_views_form_commerce_cart_form_default_alter(&$form, &$form_state, $form_id) {
  omega_benchmark_add_continue_shopping_link($form);
  return;
}

function omega_benchmark_form_commerce_checkout_form_checkout_alter(&$form, &$form_state, $form_id) {
  $form['buttons']['cancel']['#value'] = 'Edit Cart';
  $form['buttons']['cancel']['#prefix'] = '';
  $form['buttons']['continue']['#value'] = 'Review order & pay';
  $form['buttons']['continue']['#validate'][] = 'omega_benchmark_validate_commerce_checkout_form_checkout';
  
  global $user;
  if ($user->uid == 0) {
    $form['buttons']['login'] = array(
      '#prefix' => '<div class="login-user">',
      '#suffix' => '</div>',
      '#markup' => ctools_modal_text_button(t('Modal Login'), 'modal_forms/nojs/login', t('Login via modal'),  'ctools-modal-modal-popup-small checkout-login-popup'),
      '#weight' => 20,
    );
  }
  
  omega_benchmark_add_continue_shopping_link($form);
  return;
}

function omega_benchmark_validate_commerce_checkout_form_checkout(&$form, &$form_state) {
  global $user;
  //kpr($user);
  if ($user->uid == 0) {
    $login_link = ctools_modal_text_button(t('login or register'), 'modal_forms/nojs/login', t('Login'),  'ctools-modal-modal-popup-small checkout-login-popup');
    form_set_error('not_logged_in', "You must $login_link before checking out");
  }
}

function omega_benchmark_form_commerce_checkout_form_review_alter(&$form, &$form_state, $form_id) {
  $form['buttons']['back']['#value'] = t('Edit Account Information');
  $form['buttons']['back']['#prefix'] = '';
  omega_benchmark_add_continue_shopping_link($form);
  return;
}

function omega_benchmark_form_views_form_commerce_cart_block_default_alter(&$form, &$form_state, $form_id) {
  $form['output']['#prefix'] = '<div class="shopping-cart-line-items">';
  $form['output']['#suffix'] = '</div>';
  return;
}


function omega_benchmark_form_commerce_cart_add_to_cart_form_alter(&$form, &$form_state, $form_id) {
  $form['#submit'][] = '_omega_benchmark_add_to_cart_callback';
  
  if (isset($form['line_item_fields']['field_team_name']['und']['#options']['_none'])) {
    $form['line_item_fields']['field_team_name']['und']['#options']['_none'] = '- Select a Team Name -';
  }
}

function omega_benchmark_preprocess_html(&$variables) {
  drupal_add_css(path_to_theme() . '/css/iefix.css', array('group' => CSS_THEME, 'browsers' => array('IE' => 'lte IE 8', '!IE' => FALSE), 'preprocess' => FALSE));
  drupal_add_css(path_to_theme() . '/css/iefix.css', array('group' => CSS_THEME, 'browsers' => array('IE' => 'lte IE 7', '!IE' => FALSE), 'preprocess' => FALSE));
  drupal_add_css(path_to_theme() . '/css/iefix.css', array('group' => CSS_THEME, 'browsers' => array('IE' => 'IE 6', '!IE' => FALSE), 'preprocess' => FALSE));
}


function _omega_benchmark_add_to_cart_callback($form, &$form_state) {
  if ($form_state['triggering_element']['#value'] = 'Add to cart') {
    $sku = $form['line_item_fields']['commerce_product']['und']['#value'];
    
    if (isset($_SESSION['messages']['status'])) {
      foreach($_SESSION['messages']['status'] as $delta => $message) {
        $_SESSION['messages']['status'][$delta] =
          '<div class="' . strtolower($sku) . '">' . $message . '</div>';
      }
    }
  }
}
