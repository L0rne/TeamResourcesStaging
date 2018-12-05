(function ($) {
  $(document).ready(
    Drupal.behaviors.jeditable = function(context) {
        var saving = Drupal.t('Saving...');
        var edit = Drupal.t('Click to edit...');
        var cancel = Drupal.t('Cancel');
        var save = Drupal.t('Save');
        var ok = Drupal.t('OK');

      $('.jeditable-textfield').editable('/jeditable/ajax/save', {
        indicator : saving,
        tooltip   : edit,
        cancel    : cancel,
        submit    : save,
        style     : 'display: inline; min-width: 100px;'
      });
      $('.jeditable-textarea').editable('/jeditable/ajax/save', { 
        type      : 'textarea',
        cancel    : cancel,
        submit    : ok,
        indicator : saving,
        tooltip   : edit,
        height     : '80px;'
      });
      $('.jeditable-select').editable('/jeditable/ajax/save', { 
         loadurl  : '/jeditable/ajax/load',
         type     : 'select',
         submit   : ok,
         style    : 'display: inline'
      })
  });
  
})(jQuery);