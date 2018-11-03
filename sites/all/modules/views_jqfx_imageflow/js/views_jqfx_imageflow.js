// $Id$

/**
 *  @file
 *  This will pass the settings and initiate the ImageFlow.
 */
(function($) {
Drupal.behaviors.viewsJqfxImageFlow = {
  attach: function (context) {
    for (id in Drupal.settings.viewsJqfxImageFlow) {
      $('#' + id + ':not(.viewsJqfxImageFlow-processed)', context).addClass('viewsJqfxImageFlow-processed').each(function () {
	    var imageflow = new ImageFlow();
        var settings = Drupal.settings.viewsJqfxImageFlow[$(this).attr('id')];

        // Load ImageFlow
        imageflow.init(settings);
     });
    }
  }
}

})(jQuery);