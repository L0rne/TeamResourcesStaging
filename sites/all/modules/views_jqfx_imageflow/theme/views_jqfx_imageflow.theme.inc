<?php
// $Id$

/**
 *  @file
 *  Theme & preprocess functions for the Views jQFX: Imageflow module.
 */

/**
 *  We'll grab only the first image from each row.
 */
function template_preprocess_views_jqfx_imageflow(&$vars) {
  // Initialize our $images array.
  $vars['images'] = array();

  // Strip all images from the $rows created by the original view query.
  foreach($vars['rows'] as $item) {
    preg_match('@(<\s*img\s+[^>]*>)@i', $item, $matches);
    if ($image = $matches[1]) {
      // We need to add a URL to 'longdesc', as required by the plugin.
      // If our image is in an anchor tag, use its URL.
      preg_match('@<\s*a\s+href\s*=\s*"\s*([^"]+)\s*"[^>]*>[^<]*'. preg_quote($image) .'[^<]*<\s*/a\s*>@i', $item, $urls);
      if (!($url = $urls[1])) {
        // Otherwise link to the original image.
        preg_match('@src\s*=\s*"([^"]+)"@i', $image, $urls);
        if (!($url = $urls[1])) {
          // If we get this far, there are probably more serious problems.
          // But for now, we'll go to the front page instead.
          $url = url('<front>');
        }
      }

      $image = str_replace('http://'.$_SERVER['HTTP_HOST'].'/','',$image);
      // Add the URL to the image's longdesc tag.
      $image = preg_replace('@img\s+@i', 'img longdesc="'. $url .'" ', $image);

      // Add the image to our image array to display.
      $vars['images'][] = $image;
    }
  }

  $options = $vars['options']['views_jqfx_imageflow'];

  _views_jqfx_imageflow_add_js($options, 'views-jqfx-imageflow-images-'. $vars['id']);

  $vars['class'] = array(
    'views-jqfx-imageflow-images',
    'imageflow',
  );

  $vars['classes'] = implode(' ', $vars['class']);
  drupal_add_css(drupal_get_path('module', 'views_jqfx_imageflow') .'/css/views_jqfx_imageflow.css');
}

function _views_jqfx_imageflow_add_js($options, $id) {
  static $loaded_themes;

  // Find the path to our plugin.
  $path = 'sites/all/libraries';

  // Add the plugin JS and CSS.
  drupal_add_js($path .'/ImageFlow/imageflow.js');
  drupal_add_css($path .'/ImageFlow/imageflow.css');

  //Add the module JS
  $drupal_path = drupal_get_path('module', 'views_jqfx_imageflow');
  drupal_add_js($drupal_path .'/js/views_jqfx_imageflow.js');

  $settings = array(
    'ImageFlowID' => 'views-jqfx-imageflow-images-1',
    'aspect_ratio' => check_plain($options['aspect_ratio']),
    'images_height' => check_plain($options['images_height']),
    'image_cursor' => check_plain($options['image_cursor']),
    'slider_cursor' => check_plain($options['slider_cursor']),
    'slider' => $options['slider'] ? TRUE : 0,
    'start' => check_plain($options['start']),
    'buttons' => $options['buttons'] ? TRUE : 0,
    'captions' => $options['captions'] ? TRUE : 0,
    'circular' => $options['circular'] ? TRUE : 0,
    'slideshow' => $options['slideshow'] ? TRUE : 0,
    'reflections' => $options['reflections'] ? TRUE : 0,
    'reflectPath' => check_plain($options['reflectPath']),
    'imagePath' => check_plain($options['imagePath']),
    'imageFocusM' => check_plain($options['imageFocusM']),
    'scrollbarP' => check_plain($options['scrollbarP']),
    'imageFocusMax' => (integer)$options['imageFocusMax'],
    'onClick' => check_plain($options['onClick']),
  );

  drupal_add_js(array('viewsJqfxImageFlow' => array($id => $settings)), 'setting');
}
