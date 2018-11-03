<?php
// $Id$

/**
 *  Implements hook_views_jqfx_jqfx_types().
 */
function views_jqfx_imageflow_views_jqfx_jqfx_types() {
  $options = array(
    'views_jqfx_imageflow' => t('Imageflow'),
  );
  return $options;
}

/**
 *  Implements hook_views_jqfx_option_definition().
 */
function views_jqfx_imageflow_views_jqfx_option_definition() {
  // Set our default options.
  $options['views_jqfx_imageflow'] = array(
    'contains' => array(
      'aspect_ratio' => array('default' => '1.964'),
      'images_height' => array('default' => '0.67'),
      'image_cursor' => array('default' => 'pointer'),
      'slider_cursor' => array('default' => 'pointer'),
      'slider' => array('default' => TRUE),
      'start' => array('default' => 'middle'),
      'buttons' => array('default' => FALSE),
      'captions' => array('default' => TRUE),
      'circular' => array('default' => FALSE),
      'slideshow' => array('default' => FALSE),
      'reflections' => array('default' => TRUE),
      'reflectPath' => array('default' => 'sites/all/libraries/ImageFlow/'),
      'imagePath' => array('default' => '../../../../'),
      'imageFocusM' => array('default' => '1.0'),
      'scrollbarP' => array('default' => '0.6'),
      'imageFocusMax' => array('default' => '4'),
      'onClick' => array('default' => 'function() { document.location = this.url; }'),
    ),
  );
  return $options;
}

/**
 *  Implements hook_views_jqfx_views_jqfx_type_form().
 */
function views_jqfx_imageflow_views_jqfx_jqfx_type_form(&$form, &$form_state, &$view) {

  if (!file_exists('sites/all/libraries/ImageFlow/imageflow.packed.js')) {
    $form['views_jqfx_imageflow']['no_imageflow_js'] = array(
      '#markup' => '<div style="color: red">' . t('You need to download ImageFlow plugin and copy it to sites/all/libraries/ImageFlow. You can find the plugin at !url.', array('!url' => l('http://finnrudolph.de/ImageFlow/Download', 'http://finnrudolph.de/ImageFlow/Download', array('attributes' => array('target' => '_blank'))))) . '</div>',
    );
  }

  $form['views_jqfx_imageflow']['aspect_ratio'] = array(
    '#type' => 'textfield',
    '#title' => t('Aspect ratio'),
    '#description' => t('Aspect ratio of the ImageFlow container (width divided by height).'),
    '#default_value' => $view->options['views_jqfx_imageflow']['aspect_ratio'],
  );
  $form['views_jqfx_imageflow']['images_height'] = array(
    '#type' => 'textfield',
    '#title' => t('Images height'),
    '#description' => t('Height of the images div container to be multiplied.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['images_height'],
  );
  $form['views_jqfx_imageflow']['image_cursor'] = array(
    '#type' => 'select',
    '#title' => t('Image cursor'),
    '#description' => t('Cursor type for the images.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['image_cursor'],
    '#options' => array(
      'pointer' => t('pointer'),
      'default' => t('default'),
      'e-resize' => t('e-resize'),
    ),
  );
  $form['views_jqfx_imageflow']['slider'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show slider'),
    '#description' => t('Disables / enables the scrollbar.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['slider'],
  );
  $form['views_jqfx_imageflow']['slider_cursor'] = array(
    '#type' => 'select',
    '#title' => t('Slider cursor'),
    '#description' => t('Cursor type for the slider.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['slider_cursor'],
    '#options' => array(
      'pointer' => t('pointer'),
      'default' => t('default'),
      'e-resize' => t('e-resize'),
    ),
  );
  $form['views_jqfx_imageflow']['start'] = array(
    '#type' => 'select',
    '#title' => t('Start position'),
    '#description' => t('The position to begin the ImageFlow.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['start'],
    '#options' => array(
      'start' => t('Start'),
      'middle' => t('Middle'),
      'end' => t('End'),
      'random' => t('Random'),
    ),
  );
  $form['views_jqfx_imageflow']['buttons'] = array(
    '#type' => 'checkbox',
    '#title' => t('Buttons'),
    '#description' => t('Toggles navigation buttons.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['buttons'],
  );
  $form['views_jqfx_imageflow']['captions'] = array(
    '#type' => 'checkbox',
    '#title' => t('Display captions'),
    '#description' => t('Disables / enables the display of captions.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['captions'],
  );
  $form['views_jqfx_imageflow']['circular'] = array(
    '#type' => 'checkbox',
    '#title' => t('Circular'),
    '#description' => t('Toggles circular rotation.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['circular'],
  );
  $form['views_jqfx_imageflow']['slideshow'] = array(
    '#type' => 'checkbox',
    '#title' => t('Slideshow'),
    '#description' => t('Enables an slideshow mode.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['slideshow'],
  );
  $form['views_jqfx_imageflow']['reflections'] = array(
    '#type' => 'checkbox',
    '#title' => t('Display reflections'),
    '#description' => t('Disables / enables the display of reflections.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['reflections'],
  );
  $form['views_jqfx_imageflow']['reflectPath'] = array(
    '#type' => 'textfield',
    '#title' => t('Reflect path'),
    '#description' => t('Path to the reflect.php file for the image reflections. Leave to default unless you have a reason to change it'),
    '#default_value' => $view->options['views_jqfx_imageflow']['reflectPath'],
  );
  $form['views_jqfx_imageflow']['imagePath'] = array(
    '#type' => 'textfield',
    '#title' => t('Image path'),
    '#description' => t('Path to the images relative to the location of the reflect.php script. Leave to default unless you have a reason to change it'),
    '#default_value' => $view->options['views_jqfx_imageflow']['imagePath'],
  );
  $form['views_jqfx_imageflow']['imageFocusM'] = array(
    '#type' => 'textfield',
    '#title' => t('Image focus multiplier'),
    '#description' => t('Multiplier for the focussed image size.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['imageFocusM'],
  );
  $form['views_jqfx_imageflow']['scrollbarP'] = array(
    '#type' => 'textfield',
    '#title' => t('Scrollbar width multiplier'),
    '#description' => t('Multiplier width of the scrollbar (0-1).'),
    '#default_value' => $view->options['views_jqfx_imageflow']['scrollbarP'],
  );
  $form['views_jqfx_imageflow']['imageFocusMax'] = array(
    '#type' => 'textfield',
    '#title' => t('Image focus max'),
    '#description' => t('Maximum number of images on each side of the focussed one.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['imageFocusMax'],
  );
  $form['views_jqfx_imageflow']['onClick'] = array(
    '#type' => 'textarea',
    '#title' => t('onClick override'),
    '#description' => t('You may override the onClick JavaScript function of slide show image frames here.'),
    '#default_value' => $view->options['views_jqfx_imageflow']['onClick'],
  );
}