<?php
/**
 * @file
 * team_node.features.inc
 */

/**
 * Implementation of hook_node_info().
 */
function team_node_node_info() {
  $items = array(
    'team' => array(
      'name' => t('team'),
      'base' => 'node_content',
      'description' => t('Use this content type to create teams that will take the Team Benchmark diagnostics'),
      'has_title' => '1',
      'title_label' => t('Team Name'),
      'help' => t('Enter a descriptive name that identifies your team.'),
    ),
  );
  return $items;
}
