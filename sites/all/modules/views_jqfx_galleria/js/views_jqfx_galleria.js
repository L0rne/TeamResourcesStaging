// $Id$

/**
 *  @file
 *  This will pass the settings and initiate the Galleria.
 */
(function($) {
Drupal.behaviors.viewsJqfxGalleria = {
  attach: function (context) {
    for (id in Drupal.settings.viewsJqfxGalleria) {
      $('#' + id + ':not(.viewsJqfxGalleria-processed)', context).addClass('viewsJqfxGalleria-processed').each(function () {
        var settings = Drupal.settings.viewsJqfxGalleria[$(this).attr('id')];

        // Clean up the settings array by removing things that don't need to be passed
        var themeURL = settings['themeURL'];
        delete settings['themeURL'];
        var theme = settings['theme'];
        delete settings['theme'];

        // Load Galleria (the theme will be applied inline)
        $(this).galleria(settings);
     });
    }
  }
}

})(jQuery);
