<?php

/**
 * @file
 * Allows mappings of PDFs to site content
 */

/* ---------------- Configuration --------------------*/

/**
 * Settings form for user to place API Key
 */
function fillpdf_settings($form, &$form_state) {
  $form['settings_help'] = array(
    '#type' => 'markup',
    '#value' => t("
	    This module requires one of several external PDF manipulation tools -- you can:<ul>
        <li>Sign up for <a href='http://fillpdf-service.com'>Fillpdf as-a-service</a>, and plug your API key in here; <strong>or</strong>
        <li>Deploy locally -- You'll need a VPS or dedicated server so you can deploy PHP/JavaBridge on Tomcat (see README.txt), then select \"Use Local Service\"
        <li>Use a local installation of the pdftk program - you'll need a VPS or a dedicated server so you can install pdftk (see README.txt), then select \"Use locally-installed pdftk\"
      </ul>
	   "),
  );
  $form['remote'] = array(
    '#type' => 'fieldset',
    '#title' => t('Use Remote Service'),
  );
  $form['remote']['fillpdf_remote_service'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use fillpdf-service.com'),
    '#default_value' => variable_get('fillpdf_remote_service', true),
  );
  $form['remote']['fillpdf_api_key'] = array(
    '#type' => 'textfield',
    '#title' => t('API Key'),
    '#default_value' => variable_get('fillpdf_api_key', ''),
    '#description' => t(''),
  );
  if (variable_get('fillpdf_api_key', '') == '') {
    $form['remote']['warning'] = array(
      '#type' => 'markup',
      '#prefix' => "<div class='warning'>",
      '#suffix' => "</div>",
      '#value' => t("You need to sign up for an API key at <a href='http://fillpdf-service.com'>fillpdf-service.com</a>"),
    );
  }

  $form['local'] = array(
    '#type' => 'fieldset',
    '#title' => t('Use Local Service'),
  );
  $form['local']['fillpdf_local_service'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use locally-installed PHP/JavaBridge'),
    '#default_value' => (variable_get('fillpdf_local_service', true)),
  );
  if (!(file_exists(drupal_get_path('module', 'fillpdf') . '/lib/JavaBridge/java/Java.inc'))) {
    $form['local']['warning'] = array(
      '#type' => 'markup',
      '#prefix' => "<div class='warning'>",
      '#suffix' => "</div>",
      '#value' => t("JavaBridge isn't installed locally.  See README.txt for setting it up."),
    );
  }

  $form['local_php'] = array(
    '#type' => 'fieldset',
    '#title' => t('Use Local pdftk'),
  );
  $form['local_php']['fillpdf_local_php'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use locally-installed pdftk'),
    '#default_value' => (variable_get('fillpdf_local_php', true)),
  );
  //TODO: Modify to add check for pdftk installed

  $js = <<<JS
Drupal.behaviors.fillpdfSettingsCheckboxes = {
  attach: function (context) {
    (function ($) {
      $("input#edit-fillpdf-remote-service").click(function(){
        $("input#edit-fillpdf-local-service").attr('checked', false);
        $("input#edit-fillpdf-local-php").attr('checked', false);
      });
      $("input#edit-fillpdf-local-service").change(function(){
        $("input#edit-fillpdf-remote-service").attr('checked', false);
        $("input#edit-fillpdf-local-php").attr('checked', false);
      });
      $("input#edit-fillpdf-local-php").change(function(){
        $("input#edit-fillpdf-local-service").attr('checked', false);
        $("input#edit-fillpdf-remote-service").attr('checked', false);
      });
    })(jQuery);
  }
};
JS;
  drupal_add_js($js, array('type' => 'inline', 'scope' => JS_DEFAULT));

  return system_settings_form($form);
}


/* ---------------- Form Create --------------------*/

/**
 * Manage your existing forms, or upload a new one
 */
function fillpdf_forms_admin($form, &$form_state) {
  $result = db_query("SELECT title, fid FROM {fillpdf_forms} ORDER BY title");
  $header = array(t('Title'), array(
      'data' => t('Operations'),
      'colspan' => '4',
    ));
  $rows = array();
  foreach ($result as $pdf_form) {
    $row = array(
      check_plain($pdf_form->title),
      l(t('Edit'), "admin/structure/fillpdf/$pdf_form->fid"),
      l(t('Delete'), "admin/structure/fillpdf/$pdf_form->fid/delete"),
      l(t('Export field mappings'), "admin/structure/fillpdf/$pdf_form->fid/export"),
      l(t('Import field mappings'), "admin/structure/fillpdf/$pdf_form->fid/import"),
    );
    $rows[] = $row;
  }

  $form['existing_forms'] = array(
    '#markup' => theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'fillpdf'))),
  );

  $form['#attributes'] = array('enctype' => "multipart/form-data");
  $form['upload_pdf'] = array(
    '#type' => 'file',
    '#title' => 'Upload',
    '#description' => 'Upload a PDF template to create a new form',
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    '#weight' => 15,
  );

  return $form;
}

/**
 * Makes sure the Upload was provided (want to validate .pdf here too)
 */
function fillpdf_forms_admin_validate($form, &$form_state) {

  // uploading anything?
  $file = $_FILES['files']['name']['upload_pdf'];
  if (!$file) {
    form_set_error('url', t('A PDF must be provided.'));
  }

  //from includes/file.inc, line 634, but can't use that function because file not an object yet	
  if (!preg_match('/\.pdf$/i', $file)) {
    form_set_error('url', t('Only PDF files are allowed'));
  }

  // directory exist or writeable?
  $dir = file_build_uri('fillpdf');
  file_prepare_directory($dir, FILE_CREATE_DIRECTORY);

}

/**
 * Creates a new Form from the uploaded PDF, including parsed fields
 */
function fillpdf_forms_admin_submit($form, &$form_state) {
  $dir = file_build_uri('fillpdf');
  // $validators not working, so I just checked manually in fillpdf_forms_validate()
  $validators = array('file_validate_extensions' => array('pdf'));
  if ($file = file_save_upload('upload_pdf', $validators, $dir, FILE_EXISTS_REPLACE)) {
    drupal_set_message('<strong>' . $file->filename . '</strong> was successfully uploaded');
    $file->status = FILE_STATUS_PERMANENT;
    $file = file_save($file);
    // Does this file already exist in {fillpdf_forms}? If so, don't re-insert it.
    $exists = (bool) db_select('fillpdf_forms', 'ff')
      ->fields('ff', array('fid'))
      ->condition('fid', $file->fid)
      ->execute()
      ->fetchField();
    if ($exists === FALSE) {
      db_insert('fillpdf_forms')
        ->fields(array(
          'fid' => $file->fid,
          'title' => $file->filename,
          'url' => $file->uri,
        ))
        ->execute();
    }
    $fid = $file->fid;
    fillpdf_parse_pdf($fid);
  }
  else {
    //commented out because even though error if file doesn't upload right, not error if they dont' upload a file (& this is still triggered)
    drupal_set_message('Error saving file to ' . $dir, 'error');
  }

  $form_state['redirect'] = "admin/structure/fillpdf/$fid";
}


/* ---------------- Form Edit --------------------*/

/**
 * Edit existing PDF form
 */
function fillpdf_form_edit($form, &$form_state, $fid) {
  $pdf_form = db_query("SELECT * FROM {fillpdf_forms} WHERE fid = :fid", array(':fid' => $fid))->fetch();

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#maxlength' => 127,
    '#default_value' => $pdf_form->title,
    '#required' => TRUE,
  );

  // @@TODO:
  // They can upload a PDF any time, but fields will only be generated on add.  Don't want to purge existing fields,
  // however a user might have accidently uploaded an old template and discover much later (if it's substantially different, just
  // create a new Form
  $form['pdf_info'] = array(
    '#type' => 'fieldset',
    '#title' => 'PDF Form information',
    '#collapsed' => true,
  );
  $form['pdf_info']['submitted_pdf'] = array(
    '#type' => 'item',
    '#title' => t('Uploaded PDF'),
    '#description' => $pdf_form->url,
  );
  $form['pdf_info']['sample_populate'] = array(
    '#type' => 'item',
    '#title' => 'Sample PDF',
    '#description' => l("See which fields are which in this PDF", fillpdf_pdf_link($fid, null, null, true)),
  );
  $form['pdf_info']['form_id'] = array(
    '#type' => 'item',
    '#title' => 'Form Info',
    '#description' => "Form ID: [$fid].  Populate this form with node IDs, such as /fillpdf?fid=$fid&nid=10<br/>",
  );


  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  $form['delete'] = array(
    '#type' => 'submit',
    '#value' => t('Delete'),
  );
  //$form['#pdf_form'] = &$pdf_form;
  $form['#pdf_form'] = $pdf_form;

  // @@TODO: order by weight, and add dragable ala http://www.computerminds.co.uk/quick-guide-using-drupal-add-tabledrag-and-enjoying-jquery-drag-and-drop-loveliness
  $q = db_query('SELECT * FROM {fillpdf_fields} WHERE fid = :fid', array(':fid' => $fid));
  $header = array(t('Label'), t('PDF-field key'), t('Value'), array(
    'data' => t('Operations'),
    'colspan' => 2,
  ));
  $rows = array();
  foreach ($q as $field) {
    $row = array(
      check_plain($field->label), //editable
      check_plain($field->pdf_key),
      $field->value, //editable, expandable
      l(t('Edit'), "admin/structure/fillpdf/$fid/edit/{$field->pdf_key}"),
      l(t('Delete'), "admin/structure/fillpdf/$fid/delete/{$field->pdf_key}"),
    );
    $rows[] = $row;
  }

  $form['existing_fields'] = array(
    '#markup' => '<br/><br/>' . theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'fillpdf_fields'))),
  );

  //  $form['tokens_fieldset'] = array(
  //    '#type' => 'fieldset',
  //    '#title' => 'Tokens',
  //    '#collapsible' => TRUE,
  //    '#collapsed' => TRUE,
  //    '#weight' => 1,
  //  );
  //  $form['tokens_fieldset']['tokens'] = array(
  //    '#value' => theme('token_help'),
  //  );


  return $form;
}

/**
 * Submit Edit or Delete for existing PDF form
 */
function fillpdf_form_edit_submit($form, &$form_state) {
  if ($form_state['values']['op'] == t('Delete')) {
    $form_state['redirect'] = "admin/structure/fillpdf/{$form['#pdf_form']->fid}/delete";
    return;
  }
  else {
    db_update('fillpdf_forms')
  ->fields(array(
      'title' => $form_state['values']['title'],
    ))
  ->condition('fid', $form['#pdf_form']->fid)
  ->execute();
    $form_state['redirect'] = "admin/structure/fillpdf/{$form['#pdf_form']->fid}";
    drupal_set_message('Successfully updated form');
    //$form_state['nid'] = $node->nid;
  }
}

/**
 * Delete form confirmation
 */
function fillpdf_form_delete_confirm($form, &$form_state, $pdf_form) {
  if (is_numeric(arg(3))) {
    $pdf_form = db_query("SELECT * FROM {fillpdf_forms} WHERE fid = :fid", array(':fid' => arg(3)))->fetch();
  }
  if (!$pdf_form) {
    drupal_not_found();
    drupal_exit();
  }

  $form['#pdf_form'] = $pdf_form;
  return confirm_form($form,
    t('Are you sure you want to delete the form %title?', array('%title' => $pdf_form->title)),
    'admin/structure/fillpdf',
    t('Deleting a form will delete all the fields you created in it. This action cannot be undone.'),
    t('Delete'), t('Cancel')
  );
}

/**
 * Delete form submit
 */
function fillpdf_form_delete_confirm_submit($form, &$form_state) {
  db_delete('fillpdf_fields')
  ->condition('fid', $form['#pdf_form']->fid)
  ->execute();
  db_delete('fillpdf_forms')
  ->condition('fid', $form['#pdf_form']->fid)
  ->execute();
  drupal_set_message('Your form has been deleted.');
  $form_state['redirect'] = 'admin/structure/fillpdf';
}

/**
 * Export an importable array of PDF field key -> Label, Value mappings.
 * The array key is the PDF field key and the value is another array containing the label and
 * the value (properly keyed).
 *
 * @param mixed $pdf_form The FillPDF form ID.
 */
function fillpdf_form_export($pdf_form) {
  if (is_numeric($pdf_form)) {
    $fid = db_query("SELECT * FROM {fillpdf_forms} WHERE fid = :fid", array(':fid' => $pdf_form))->fetch();
  }
  if (!$fid) {
    drupal_not_found();
    drupal_exit();
  }
  $fields = db_query('SELECT * FROM {fillpdf_fields} WHERE fid = :fid', array(':fid' => $fid->fid));
  $export_array = array();
  foreach ($fields as $field) {
    $export_array[$field->pdf_key] = array(
      'label' => $field->label,
      'value' => $field->value,
    );
  }
  $fillpdf_code = fillpdf_form_export_encode($export_array);
  return drupal_get_form('fillpdf_export_form', $fillpdf_code);
}

/**
 * Form for exporting Fill PDF field mappings
 */
function fillpdf_export_form($form, $form_state, $code) {
  $form = array();
  $form['export'] = array(
    '#type' => 'textarea',
    '#title' => t('Fill PDF Form Mappings'),
    '#default_value' => $code,
    '#rows' => 30,
    '#description' => t('Copy this code and then on the site you want to import to, go to the Edit page for the Fill PDF form for which you want to import these mappings, and paste it in there.'),
    '#attributes' => array(
      'style' => 'width: 97%;',
    ),
  );
  return $form;
}

/**
 * Build Fill PDF configuration code string recursively
 * Code borrowed from Node Export (http://drupal.org/project/node_export). Thanks, Node Export!
 */
function fillpdf_form_export_encode($var, $iteration = 0) {
  $tab = '';
  for ($i = 0; $i < $iteration; $i++) {
    $tab = $tab . "  ";
  }
  $iteration++;
  if (is_object($var)) {
    $var = (array) $var;
    $var['#_export_fillpdf_form_encode_object'] = '1';
  }
  if (is_array($var)) {
    $empty = empty($var);
    $code = "array(" . ($empty ? '' : "\n");
    foreach ($var as $key => $value) {
      $out = $tab . "  '" . $key . "' => " . fillpdf_form_export_encode($value, $iteration) . ",\n";
      drupal_alter('fillpdf_form_export_encode', $out, $tab, $key, $value, $iteration);
      $code .= $out;
    }
    $code .= ($empty ? '' : $tab) . ")";
    return $code;
  }
  else {
    if (is_string($var)) {
      return "'" . addslashes($var) . "'";
    }
    elseif (is_numeric($var)) {
      return $var;
    }
    elseif (is_bool($var)) {
      return ($var ? 'TRUE' : 'FALSE');
    }
    else {
      return 'NULL';
    }
  }
}

/**
 * Based loosely on Node Export's import form. Import the code and configure the DB settings.
 *
 * @param mixed $form_state
 * @param mixed $pdf_form
 */
function fillpdf_form_import_form($form, &$form_state, $pdf_form) {
  if (is_numeric($pdf_form)) {
    $fid = db_query("SELECT * FROM {fillpdf_forms} WHERE fid = :fid", array(':fid' => $pdf_form))->fetch();
  }
  if (!$fid) {
    drupal_not_found();
    drupal_exit();
  }
  $form['fid'] = array(
    '#type' => 'value',
    '#value' => $fid->fid,
  );
  $form['paste'] = array(
    '#type' => 'fieldset',
    '#title' => t('Paste code'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['paste']['code'] = array(
    '#type' => 'textarea',
    '#default_value' => '',
    '#rows' => 30,
    '#description' => t('Cut and paste the results of a <em>Fill PDF Field Mappings export</em> here.'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
    '#suffix' => l(t('Reset the form'), $_GET['q']),
  );
  return $form;
}

/**
 * Check the syntax of the code the user is trying to import
 */
function fillpdf_form_import_form_validate($form, &$form_state) {
  $mappings = fillpdf_form_export_decode($form_state['values']['code']);
  if (!is_array($mappings) || empty($mappings)) {
    form_set_error('code', t('There was a problem processing your Fill PDF Field Mappings code. Please do a fresh export from the source and try pasting it again.'));
  }
  else {
    $form_state['values']['mappings'] = $mappings;
  }
}

/**
 * Perform the import.
 */
function fillpdf_form_import_form_submit($form, &$form_state) {
  $pdf_form = new stdClass();
  $pdf_form->fid = $form_state['values']['fid'];
  $mappings = $form_state['values']['mappings'];
  $fields = fillpdf_get_fields($pdf_form->fid);
  $field_keys = array_keys($fields);
  // Process the mappings
  foreach ($mappings as $pdf_key => $field_settings) {
    if (in_array($pdf_key, $field_keys)) {
      $field_settings = (object) $field_settings;
      $field_settings->pdf_key = $pdf_key;
      fillpdf_update_field($pdf_form, $field_settings, $pdf_key);
    }
    else {
      drupal_set_message(t('Your code contained field mappings for the PDF field key <em>@pdf_key</em>, but it does not exist on this form. Therefore, it was ignored.', array('@pdf_key' => $pdf_key)), 'warning');
    }
  }
  drupal_set_message(t('Successfully imported matching PDF field keys. If any field mappings failed to import, they are listed above.'));
  $form_state['redirect'] = "admin/structure/fillpdf/{$pdf_form->fid}";
}

/**
 * Evaluate and return decoded string. Borrowed from Node Export module.
 */
function fillpdf_form_export_decode($string) {
  $array = eval('return ' . $string . ';');
  // $return = fillpdf_form_export_decode_objects($array); // (wizonesolutions) Not needed since we simply process an array upon import.
  $return = $array;
  return $return;
}

/* ---------------- Fields Edit --------------------*/


/**
 * Retrieve a field from a PDF for use in editing form.
 */
function fillpdf_field($op, $fid, $pdf_key = NULL) {
  if (is_numeric($fid)) {
    $pdf_form = db_query("SELECT * FROM {fillpdf_forms} WHERE fid = :fid", array(':fid' => $fid))->fetch();
  }
  if (!$pdf_form) {
    drupal_not_found();
    drupal_exit();
  }

  if ($op == 'add') {
    drupal_set_title($pdf_form->title);
  }
  else if ($op != 'edit') {
    return fillpdf_form_overview($pdf_form);
  }
  else if ($pdf_key) {
    $field = db_query("SELECT * FROM {fillpdf_fields} WHERE pdf_key = :pdf_key AND fid = :fid", array(':pdf_key' => $pdf_key, ':fid' => $fid))->fetch();
    if (!$field) {
      drupal_not_found();
      drupal_exit();
    }
    drupal_set_title($field->label);
  }

  return drupal_get_form('fillpdf_field_edit', $pdf_form, $field);
}

/**
 * Edit a single field.
 */
function fillpdf_field_edit($form, &$form_state, $pdf_form, $field) {
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#maxlength' => 255,
    '#default_value' => $field->label,
    '#description' => t('An optional label to help you identify the field.'),
    '#weight' => 0,
  );
  $form['pdf_key'] = array(
    '#type' => 'textfield',
    '#title' => t('PDF Key'),
    '#maxlength' => 255,
    '#default_value' => $field->pdf_key,
    '#required' => TRUE,
    '#description' => t('The field key from the original PDF form.  You likely need Acrobat Pro to discover this.'),
    '#weight' => 1,
  );
  $form['value'] = array(
    '#type' => 'textarea',
    '#title' => t('Value'),
    '#default_value' => $field->value,
    '#description' => t('The content that will populate this field when the PDF is printed/saved.  This content pulls data via tokens, see below for available tokens.'),
    '#weight' => 4,
  );
  $form['tokens_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => 'Tokens',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 5,
  );
  $form['tokens_fieldset']['tokens'] = array(
    '#theme' => 'token_tree',
    '#token_types' => array('node', 'webform'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    '#weight' => 9,
  );

  if ($field) {
    $form['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#weight' => 10,
    );
  }

  $form['#pdf_field'] = $field;
  $form['#pdf_form'] = $pdf_form;

  return $form;
}

/**
 * Validate the edited field.
 */
function fillpdf_field_edit_validate($form, &$form_state) {
  $pdf_form = db_query("SELECT * FROM {fillpdf_fields} WHERE fid = :fid AND pdf_key = :pdf_key", array(':fid' => $form['#pdf_form']->fid, ':pdf_key' => $form_state['values']['pdf_key']))->fetch();
  if ($pdf_form) {
    if ($form['#pdf_field'] && $form['#pdf_field']->pdf_key == $form_state['values']['pdf_key'] ) {
      return;
    }
    else {
      form_set_error('pdf_key', t('A field with this pdf_key already exists. Choose another pdf_key.'));
    }
  }
}

/**
 * Save changes to the database.
 */
function fillpdf_field_edit_submit($form, &$form_state) {
  if ($form['#pdf_field']) {
    if ($form_state['values']['op'] == t('Delete')) {
      $form_state['redirect'] = 'admin/structure/fillpdf/' . $form['#pdf_form']->fid . '/delete/' . $form['#pdf_field']->pdf_key;
      return;
    }
    $edit_field = (object) $form_state['values'];
    fillpdf_update_field($form['#pdf_form'], $edit_field, $form['#pdf_field']->pdf_key);
  }
  else {
    //add a new field
    $edit_field = (object) $form_state['values'];
    db_insert('fillpdf_fields')
    ->fields(array(
      'fid' => $form_state['values']['#pdf_form']->fid,
      'label' => $form_state['values']['label'],
      'pdf_key' => $form_state['values']['pdf_key'],
      'value' => $form_state['values']['value'],
    ))
    ->execute();
  }
  $form_state['redirect'] = 'admin/structure/fillpdf/' . $form['#pdf_form']->fid;
}


/**
 * Delete form.
 */
function fillpdf_field_delete_confirm($form, &$form_state, $fid, $pdf_key) {
  $pdf_form = db_query("SELECT * FROM {fillpdf_forms} WHERE fid = :fid", array(':fid' => $fid))->fetch();

  if ($pdf_key) {
    $field = db_query("SELECT * FROM {fillpdf_fields} WHERE pdf_key = :pdf_key AND fid = :fid", array(':pdf_key' => $pdf_key, ':fid' => $fid))->fetch();
  }
  if (!$field) {
    drupal_not_found();
    drupal_exit();
  }

  $form['#pdf_field'] = $field;
  $form['#pdf_form'] = $pdf_form;

  return confirm_form($form,
    t('Are you sure you want to delete the field %pdf_key?', array('%pdf_key' => $field->pdf_key)),
    'admin/structure/fillpdf/' . $pdf_form->fid,
    t('This action cannot be undone.'), t('Delete'), t('Cancel')
  );
}

/**
 * Process the deletion.
 */
function fillpdf_field_delete_confirm_submit($form, &$form_state) {
  db_delete('fillpdf_fields')
  ->condition('fid', $form['#pdf_field']->fid)
  ->condition('pdf_key', $form['#pdf_field']->pdf_key)
  ->execute();
  drupal_set_message('Your field has been deleted.');
  //return 'admin/structure/fillpdf/'. $form['#pdf_field']->fid;
  $form_state['redirect'] = 'admin/structure/fillpdf/' . $form['#pdf_field']->fid;
}

/**
 * Stores the updated $field in the database
 */
function fillpdf_update_field(&$pdf_form, &$field, $old_key) {
  db_update('fillpdf_fields')
  ->fields(array(
    'label' => $field->label,
    'pdf_key' => $field->pdf_key,
    'value' => $field->value,
  ))
  ->condition('fid', $pdf_form->fid)
  ->condition('pdf_key', $old_key)
  ->execute();
}
