<?php
// $Id$

/**
 * @file
 * Admin page callback for the pchart_api module.
 */

/**
 * Builds and returns the pchart_api settings form.
 */
function pchart_api_admin_settings() {
  $form['pchart_api_image_directory'] = array(
    '#type' => 'textfield',
    '#title' => t('Default Theme'),
    '#default_value' => variable_get('pchart_api_image_directory', 'sites/default/files/'),
    '#description' => t('This setting configures the directory used for chart images. <strong>You must include the trailing slash!</strong>'),
    '#required' => TRUE,
  );
  return system_settings_form($form);
}