/**
 * @file
 * Javascript for Benchmark Data Color Threshold.
 */

/**
 * Provides a farbtastic colorpicker for the fancier widget.
 */
(function ($) {
  Drupal.behaviors.field_example_colorpicker = {
    attach: function(context) {
      $(".edit-data-color-colorpicker").live("focus", function(event) {
        var edit_field = this;
        var picker = $(this).closest('div').parent().find(".edit-data-color-color");
        
        // Hide all color pickers except this one.
        $(".edit-data-color-colorpicker").hide();
        $(picker).show();
        $.farbtastic(picker, function(color) {
          edit_field.value = color;
        }).setColor(edit_field.value);
      });
    }
  }
})(jQuery);
