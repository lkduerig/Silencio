<?php

/**
 * @file
 * Contains CER's implementation of the Hierarchical Select API.
 */

/**
 * Implements hook_hierarchical_select_params().
 */
function cer_hierarchical_select_params() {
  return CerFieldHierarchy::getParameters();
}

/**
 * Implements hook_hierarchical_select_root_level().
 */
function cer_hierarchical_select_root_level($parameters) {
  return CerFieldHierarchy::get($parameters['id'])->getOptions(CerFieldHierarchy::ROOT);
}

/**
 * Implements hook_hierarchical_select_children().
 */
function cer_hierarchical_select_children($parent, $parameters) {
  return CerFieldHierarchy::get($parameters['id'])->getOptions($parent);
}

/**
 * Implements hook_hierarchical_select_lineage().
 */
function cer_hierarchical_select_lineage($item, $parameters) {
  return CerFieldHierarchy::get($parameters['id'])->getLineage($item);
}

/**
 * Implements hook_hierarchical_select_valid_item().
 */
function cer_hierarchical_select_valid_item($item, $parameters) {
  return CerFieldHierarchy::get($parameters['id'])->validItem($item);
}

/**
 * Implements hook_hierarchical_select_item_get_label().
 */
function hook_hierarchical_select_item_get_label($item, $parameters) {
  return CerFieldHierarchy::get($parameters['id'])->getLabel($item);
}
