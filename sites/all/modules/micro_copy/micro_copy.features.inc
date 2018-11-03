<?php
/**
 * @file
 * micro_copy.features.inc
 */

/**
 * Implementation of hook_node_info().
 */
function micro_copy_node_info() {
  $items = array(
    'micro_copy' => array(
      'name' => t('Micro Copy'),
      'base' => 'node_content',
      'description' => t('Add small amounts of copy to be used as UI instructions on various pages of the website.'),
      'has_title' => '1',
      'title_label' => t('Title'),
      'help' => '',
    ),
  );
  return $items;
}
