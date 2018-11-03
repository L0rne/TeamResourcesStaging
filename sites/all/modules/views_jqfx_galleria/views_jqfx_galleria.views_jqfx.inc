<?php
// $Id$

/**
 *  Implements hook_views_jqfx_jqfx_types().
 */
function views_jqfx_galleria_views_jqfx_jqfx_types() {
  $options = array(
    'views_jqfx_galleria' => t('Galleria'),
  );
  return $options;
}

function views_jqfx_galleria_views_jqfx_option_definition() {
  $options['views_jqfx_galleria'] = array(
    'contains' => array(
      'theme' => array('default' => 'classic'),
      'custom_theme_options' => array('default' => array('custom_theme' => array('default' => 'classic'),'theme_path' => array('default' => '/sites/all/libraries/aino-galleria/themes'))),
      'autoplay' => array('default' => 0),
      'autoplay_ms' => array('default' => 3000),
      'carousel' => array('default' => TRUE),
      'carousel_follow' => array('default' => TRUE),
      'carousel_speed' => array('default' => 200),
      'carousel_steps' => array('default' => 'auto'),
      'height' => array('default' => 'auto'),
      'width' => array('default' => 'auto'),
      'data_config' => array('default' => ''),
      'data_image_selector' => array('default' => 'img'),
      'data_source' => array('default' => ''),
      'data_type' => array('default' => 'auto'),
      'debug' => array('default' => 0),
      'extend' => array('default' => ''),
      'image_crop' => array('default' => 0),
      'image_margin' => array('default' => 0),
      'image_position' => array('default' => 'center'),
      'max_scale_ratio' => array('default' => ''),
      'popup_links' => array('default' => 0),
      'preload' => array('default' => 2),
      'queue' => array('default' => TRUE),
      'show' => array('default' => 0),
      'theme' => array('default' => 'classic'),
      'thumb_crop' => array('default' => TRUE),
      'thumb_margin' => array('default' => 0),
      'thumbnails' => array('default' => TRUE),
      'transition' => array('default' => 'slide'),
      'transition_speed' => array('default' => 400),
      'avoid_flash_of_content' => array('default' => TRUE),
    ),
  );
  return $options;
}

/**
 *  Implements hook_views_jqfx_views_jqfx_type_form().
 */
function views_jqfx_galleria_views_jqfx_jqfx_type_form(&$form, &$form_state, &$view) {
  ctools_include('dependent');
  // Create the form elements for our Galleria view options.

  $options = array(
    'classic' => t('Classic'),
    'fullscreen' => t('Fullscreen'),
  );
  $form['views_jqfx_galleria']['theme'] = array(
    '#type' => 'radios',
    '#title' => t('Theme'),
    '#description' => t('Galleria theme to load on display. If you choose the %custom option, you must specify the path to your custom JavaScript theme below.', array('%custom' => t('<Custom>'))),
    '#default_value' => $view->options['views_jqfx_galleria']['theme'],
    '#options' => $options,
  );
  $form['views_jqfx_galleria']['autoplay'] = array(
    '#type' => 'checkbox',
    '#title' => t('Autoplay'),
    '#description' => t('If checked, then the slide show will begin rotating after the specified time below.'),
    '#default_value' => $view->options['views_jqfx_galleria']['autoplay'],
  );
  $form['views_jqfx_galleria']['autoplay_ms'] = array(
    '#type' => 'textfield',
    '#title' => t('Autoplay time'),
    '#description' => t('If the slide show is set to autoplay above, then begin after this many miliseconds.'),
    '#default_value' => $view->options['views_jqfx_galleria']['autoplay_ms'],
  );
  $form['views_jqfx_galleria']['carousel'] = array(
    '#type' => 'checkbox',
    '#title' => t('Carousel'),
    '#description' => t('If checked, this will activate the carousel when needed. Otherwise it will not appear at all.'),
    '#default_value' => $view->options['views_jqfx_galleria']['carousel'],
  );
  $form['views_jqfx_galleria']['carousel_follow'] = array(
    '#type' => 'checkbox',
    '#title' => t('Carousel follow'),
    '#description' => t('If checked, the carousel will follow the active image.'),
    '#default_value' => $view->options['views_jqfx_galleria']['carousel_follow'],
  );
  $form['views_jqfx_galleria']['carousel_speed'] = array(
    '#type' => 'textfield',
    '#title' => t('Carousel speed'),
    '#description' => t('The slide speed of the carousel in milliseconds.'),
    '#default_value' => $view->options['views_jqfx_galleria']['carousel_speed'],
  );
  $form['views_jqfx_galleria']['carousel_steps'] = array(
    '#type' => 'textfield',
    '#title' => t('Carousel steps'),
    '#description' => t('The number of "steps" the carousel will slide when navigating between available thumbnails. Specifying %auto will move the carousel as many steps as there are visible thumbnails.', array('%auto' => 'auto')),
    '#default_value' => $view->options['views_jqfx_galleria']['carousel_steps'],
  );
  $form['views_jqfx_galleria']['height'] = array(
    '#type' => 'textfield',
    '#title' => t('Height'),
    '#description' => t('This will set a height to the gallery. If you set this to %auto and no CSS height is found, Galleria will try to automatically add a 16/9 ratio as a fallback.', array('%auto' => 'auto')),
    '#default_value' => $view->options['views_jqfx_galleria']['height'],
  );
  $form['views_jqfx_galleria']['width'] = array(
    '#type' => 'textfield',
    '#title' => t('Width'),
    '#description' => t('This will set a width to the gallery. If you set this to %auto and no CSS height is found, Galleria will try to automatically add a 16/9 ratio as a fallback.', array('%auto' => 'auto')),
    '#default_value' => $view->options['views_jqfx_galleria']['width'],
  );
  $form['views_jqfx_galleria']['image_crop'] = array(
    '#type' => 'checkbox',
    '#title' => t('Image crop'),
    '#description' => t('If checked, all images will be scaled to fill the stage, centered and cropped. Otherwise, they will scale down so the entire image fits.'),
    '#default_value' => $view->options['views_jqfx_galleria']['image_crop'],
  );
  $form['views_jqfx_galleria']['image_margin'] = array(
    '#type' => 'textfield',
    '#title' => t('Image margin'),
    '#description' => t('Sets a margin between the image and stage. Specify the number of pixels.'),
    '#default_value' => $view->options['views_jqfx_galleria']['image_margin'],
  );
  $form['views_jqfx_galleria']['image_position'] = array(
    '#type' => 'textfield',
    '#title' => t('Image position'),
    '#description' => t("Positions the main image. Works like the CSS background-position property; i.e., 'top right' or '20% 100%'. You can use keywords, percents or pixels. The first value is the horizontal position and the second is the vertical. Read more at !read_more.", array('!read_more' => l('http://www.w3.org/TR/REC-CSS1/#background-position', 'http://www.w3.org/TR/REC-CSS1/#background-position', array('attributes' => array('target' => '_blank'))))),
    '#default_value' => $view->options['views_jqfx_galleria']['image_position'],
  );
  $form['views_jqfx_galleria']['max_scale_ratio'] = array(
    '#type' => 'textfield',
    '#title' => t('Max scale ratio'),
    '#description' => t("Sets the maximum scale ratio for images. If you don't want Galleria to upscale any images, set this to 1. Leaving it blank will allow any scaling of the images."),
    '#default_value' => $view->options['views_jqfx_galleria']['max_scale_ratio'],
  );
  $form['views_jqfx_galleria']['popup_links'] = array(
    '#type' => 'checkbox',
    '#title' => t('Popup links'),
    '#description' => t('Checking this box will open any image links in a new window.'),
    '#default_value' => $view->options['views_jqfx_galleria']['popup_links'],
  );
  $form['views_jqfx_galleria']['preload'] = array(
    '#type' => 'textfield',
    '#title' => t('Preload'),
    '#description' => t("Defines how many images Galleria should preload in advance. Please note that this only applies when you are using separate thumbnail files. Galleria always cache all preloaded images. <ul><li>%2 preloads the next 2 images in line.</li><li>%all forces Galleria to start preloading all images. This may slow down client.</li><li>%0 will not preload any images</li></ul>", array('%2' => '2', '%all' => 'all', '%0' => '0')),
    '#default_value' => $view->options['views_jqfx_galleria']['preload'],
  );
  $form['views_jqfx_galleria']['queue'] = array(
    '#type' => 'checkbox',
    '#title' => t('Queue'),
    '#description' => t("Galleria queues all activation clicks (next/prev & thumbnails). You can see this effect when, for example, clicking %next many times. If you don't want Galleria to queue, then uncheck the box.", array('%next' => t('next'))),
    '#default_value' => $view->options['views_jqfx_galleria']['queue'],
  );
  $form['views_jqfx_galleria']['show'] = array(
    '#type' => 'textfield',
    '#title' => t('Show'),
    '#description' => t("This defines what image index to show at first. If you have left the %history box checked, then a permalink will override this number.", array('%history' => t('History permalinks'))),
    '#default_value' => $view->options['views_jqfx_galleria']['show'],
  );
  $form['views_jqfx_galleria']['thumb_crop'] = array(
    '#type' => 'checkbox',
    '#title' => t('Thumb crop'),
    '#description' => t("Same as %image_crop, but for thumbnails.", array('%image_crop' => t('Image crop'))),
    '#default_value' => $view->options['views_jqfx_galleria']['thumb_crop'],
  );
  $form['views_jqfx_galleria']['thumb_margin'] = array(
    '#type' => 'textfield',
    '#title' => t('Image margin'),
    '#description' => t("Same as %image_margin, but for thumbnails.", array('%image_margin' => t('Image margin'))),
    '#default_value' => $view->options['views_jqfx_galleria']['thumb_margin'],
  );
  $options = array(
    'empty' => t('Empty'),
    FALSE => t('False'),
    TRUE => t('True'),
  );
  $form['views_jqfx_galleria']['thumbnails'] = array(
    '#type' => 'select',
    '#title' => t('Thumbnails'),
    '#description' => t("Sets the creation of thumbnails. If set to %false, then Galleria will not create thumbnails. If you set this to %empty, Galleria will create empty spans with the className %img instead of thumbnails.", array('%empty' => t('Empty'), '%image' => t('img'), '%false' => t('False'))),
    '#default_value' => $view->options['views_jqfx_galleria']['thumbnails'],
    '#options' => $options,
  );
  $options = array(
    'fade' => t('Fade'),
    'flash' => t('Flash'),
    'slide' => t('Slide'),
    'fadeslide' => t('Fade/Slide'),
  );
  $form['views_jqfx_galleria']['transition'] = array(
    '#type' => 'select',
    '#title' => t('Transition'),
    '#description' => t("The transition that is used when displaying the images.<ul><li>%fade will fade between images.</li><li>%flash will fade into the background color between images.</li><li>%slide will slide the images using the Galleria easing depending on image position.</li><li>%fadeslide will fade between images and slide slightly at the same time.</li></ul>", array('%fade' => t('Fade'), '%flash' => t('Flash'), '%slide' => t('Slide'), '%fadeslide' => t('Fade/Slide'))),
    '#default_value' => $view->options['views_jqfx_galleria']['transition'],
    '#options' => $options,
  );
  $form['views_jqfx_galleria']['transition_speed'] = array(
    '#type' => 'textfield',
    '#title' => t('Transition speed'),
    '#description' => t("The milliseconds used when applying the transition."),
    '#default_value' => $view->options['views_jqfx_galleria']['transition_speed'],
  );
  $form['views_jqfx_galleria']['advanced']['avoid_flash_of_content'] = array(
    '#type' => 'checkbox',
    '#title' => t('Avoid flash of content'),
    '#description' => t('If checked, then the images of the Galleria slide show will be hidden by JavaScript in the page header, so that there is no flash of content on the page load.'),
    '#default_value' => $view->options['views_jqfx_galleria']['advanced']['avoid_flash_of_content'],
  );
}
