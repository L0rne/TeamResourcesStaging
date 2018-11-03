// $Id$

/**
 *  @file
 *  This will pass the settings and initiate the Carousel.
 */
(function($) {
Drupal.behaviors.viewsJqfxCloudCarousel = {
  attach: function (context) {
    for (id in Drupal.settings.viewsJqfxCloudCarousel) {
      $('#' + id + ':not(.viewsJqfxCloudCarousel-processed)', context).addClass('viewsJqfxCloudCarousel-processed').each(function () {
      
        // Add the cloud_carousel class to our images for the plugin
        $('.views-jqfx-cloud-carousel img').addClass('cloudcarousel');
      
        // Get our settings
        var settings = Drupal.settings.viewsJqfxCloudCarousel[$(this).attr('id')];
        
        // Add the control and text containers to our settings
        settings.buttonLeft = $('#cloud-carousel-left-but');
        settings.buttonRight = $('#cloud-carousel-right-but');
        settings.altBox = $('#cloud-carousel-alt-text');
        settings.titleBox = $('#cloud-carousel-title-text');
        
        // Load 3d Cloud Carousel
        $(this).CloudCarousel(settings);
      });
    }
  }
}

})(jQuery);
