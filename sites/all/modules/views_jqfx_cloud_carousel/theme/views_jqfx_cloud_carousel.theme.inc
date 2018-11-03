<?php
// $Id$

/**
 * @file
 * Theme & preprocess functions for the Views jQFX: Cloud Carousel module.
 */

function template_preprocess_views_jqfx_cloud_carousel(&$vars) {
  // Initialize our $images array.
  $vars['images'] = array();

  // Strip the images from the $rows created by the original view query.
  foreach ($vars['rows'] as $item) {
    // For our images we'll look for items enclosed in anchor tags first.
    if (preg_match('@(<a.*?img.*?</a>)@i', $item, $matches)) {
      $image = $matches[1];
    }
    // If no anchor tags we'll just look for images.
    elseif (preg_match('@(<\s*img\s+[^>]*>)@i', $item, $matches)) {
      $image = $matches[1];
    }
    // If we have no images then we have no Carousel.
    else {$images = NULL;}
    // Add the image to our image array to display.
    $vars['images'][] = $image;
  }

  $options = $vars['options']['views_jqfx_cloud_carousel'];
  
  // Add the module css
  drupal_add_css((drupal_get_path('module', 'views_jqfx_cloud_carousel')) . '/theme/styles/' . $options['style'] . '/views_jqfx_cloud_carousel_' . $options['style'] . '.css');

  _views_jqfx_cloud_carousel_add_js($options, 'views-jqfx-cloud-carousel-images-' . $vars['id']);
  
}

function _views_jqfx_cloud_carousel_add_js($options, $id) {

  // Find the path to our plugin.
  $path = 'sites/all/libraries';

  // Add the plugin JS.
  drupal_add_js($path . '/cloud-carousel/cloud-carousel.js');
  
  // If the mousewheel option is true add its javascript
  if ($options['mouse_wheel']) {
    drupal_add_js($path . '/jquery.mousewheel/jquery.mousewheel.js');
  }

  // Get the javascript settings.
  $settings = array(
    'autoRotate'       => $options['auto_rotate'],
    'autoRotateDelay'  => (integer)$options['auto_rotate_delay'],
    'bringToFront'     => $options['bring_to_front'] ? TRUE : FALSE,
    'FPS'              => (integer)$options['fps'],
    'minScale'         => (float)$options['min_scale'],
    'mouseWheel'       => $options['mouse_wheel'] ? TRUE : FALSE,
    'openColorbox'     => $options['open_colorbox'] ? TRUE : FALSE,
    'reflHeight'       => (integer)$options['refl_height'],
    'reflGap'          => (integer)$options['refl_gap'],
    'reflOpacity'      => (float)$options['refl_opacity'],
    'speed'            => (float)$options['speed'],
    'xPos'             => (integer)$options['x_pos'],
    'yPos'             => (integer)$options['y_pos'],
    'xRadius'          => (integer)$options['x_radius'],
    'yRadius'          => (integer)$options['y_radius'],
//    'buttonLeft'       => '$("#left-but")',
//    'buttonRight'      => '$("#right-but")',
//    'altBox'           => '$("#alt-text")',
//    'titleBox'         => '$("#title-text")',
  );
  
  // Add the settings array.
  drupal_add_js(array('viewsJqfxCloudCarousel' => array($id => $settings)), 'setting');
  
  // Add the module javascript
  $drupal_path = drupal_get_path('module', 'views_jqfx_cloud_carousel');
  drupal_add_js($drupal_path . '/js/views_jqfx_cloud_carousel.js');
  
  // JS for the page footer to help with FUOC
//  $fuoc = '$(document)ready(function() {$(".views-jqfx-cloud-carousel-container")show();});';
//  drupal_add_js($fuoc, array('type' => 'inline', 'scope' => 'footer', 'weight' => 5));
}
