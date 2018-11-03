<?php
// $Id$

/**
 *  @file
 *  Theme & preprocess functions for the Views jQfx: Galleria module.
 */

/**
 *  We'll grab only the first image from each row.
 */
function template_preprocess_views_jqfx_galleria(&$vars) {
  // Initialize our $images array.
  $vars['images'] = array();

  $vars['original_rows'] = $vars['rows'];

  // Strip all images from the $rows created by the original view query.
  foreach($vars['rows'] as $id => $item) {
    preg_match('@(<\s*img\s+[^>]*>)@i', $item, $matches);
    if ($image = $matches[1]) {
      // If our image is in an anchor tag, use its URL.
      // Note that we'll be assigning the value of $url during the comparison.
      // @see http://www.php.net/manual/en/language.operators.assignment.php.
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

      // Ensure the link for the original image is preserved.
      // $url has already been url'ized.
      $vars['rows'][$id] = '<a href="'. $url .'">'. $image .'</a>';

      // Add the image to our image array to display.
      $vars['images'][$id] = $image;
    }
  }

  $options = $vars['options']['views_jqfx_galleria'];

  _views_jqfx_galleria_add_js($options, 'views-jqfx-galleria-images-'. $vars['id']);

  $vars['class'] = array(
    'views-jqfx-galleria-images',
    'galleria',
  );

  // Avoid flash of content.
  if ($options['advanced']['avoid_flash_of_content']) {
    $vars['class'][] = 'hidden';
  }

  $vars['classes'] = implode(' ', $vars['class']);
  drupal_add_css(drupal_get_path('module', 'views_jqfx_galleria') .'/css/views_jqfx_galleria.css');
}

function _views_jqfx_galleria_add_js($options, $id) {
  static $loaded_themes;

  // Keep track of which Galleria plugin JS themes have already been loaded.
  if (!isset($loaded_themes)) {
    $loaded_themes = array();
    $javascript = '';
  }
  
  // Find the path to our plugin.
  $path = 'sites/all/libraries';

  // Add the plugin JS.
  drupal_add_js($path .'/aino-galleria/src/galleria.js');

  $drupal_path = drupal_get_path('module', 'views_jqfx_galleria');
  drupal_add_js($drupal_path .'/js/views_jqfx_galleria.js');

  $settings = array(
    'theme' => check_plain($options['theme']),
    'themeURL' => 'sites/all/libraries/aino-galleria/src/themes/classic/galleria.classic.js',
    'carousel' => $options['carousel'] ? TRUE : 0,
    'carousel_speed' => (integer)$options['carousel_speed'],
    'image_crop' => $options['image_crop'] ? TRUE : 0,
    'image_margin' => (integer)$options['image_margin'],
    'image_position' => check_plain($options['image_position']),
    'popup_links' => $options['popup_links'] ? TRUE : 0,
    'queue' => $options['queue'] ? TRUE : 0,
    'show' => (integer)$options['show'],
    'thumb_crop' => $options['thumb_crop'] ? TRUE : 0,
    'thumb_margin' => (integer)$options['thumb_margin'],
    'thumbnails' => $options['thumbnails'] ? TRUE : 0,
    'transition' => check_plain($options['transition']),
    'transition_speed' => (integer)$options['transition_speed'],
  );

  //the theme files from the plugin
  drupal_add_js($path .'/aino-galleria/src/themes/'.$settings['theme'].'/galleria.'.$settings['theme'].'.js');
  drupal_add_css($path .'/aino-galleria/src/themes/'.$settings['theme'].'/galleria.'.$settings['theme'].'.css');

  if ($settings['themeURL']) {
    $javascript .= ' Galleria.loadTheme("'. $settings['themeURL'] .'");';
    $loaded_themes[$settings['themeURL']] = TRUE;
  }

  // Autoplay is FALSE or the number of MS.
  $settings['autoplay'] = ($options['autoplay'] && (integer)$options['autoplay_ms']) ? (integer)$options['autoplay_ms'] : 0;

  // Carousel_steps is 'auto' or an integer.
  $settings['carousel_steps'] = ($options['carousel_steps'] == 'auto') ? 'auto' : (integer)$options['carousel_steps'];

  // Height is 'auto' or an integer.
  if (($options['height'] == 'auto') || !(integer)$options['height']) {
    $settings['height'] = 'auto';
  }
  else {
    $settings['height'] = (integer)$options['height'];
  }

  if ($options['max_scale_ratio'] && is_numeric($options['max_scale_ratio'])) {
    $settings['max_scale_ratio'] = (integer)$options['max_scale_ratio'];
  }

  $settings['preload'] = ($options['preload'] == 'auto') ? 'auto' : (integer)$options['preload'];

  drupal_add_js(array('viewsJqfxGalleria' => array($id => $settings)), 'setting');
  
  // Output our custom javascript
  drupal_add_js($javascript, 'inline');
}
