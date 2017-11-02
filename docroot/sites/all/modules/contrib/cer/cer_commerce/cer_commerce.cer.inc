<?php

/**
 * Implements hook_cer_fields().
 */
function cer_commerce_cer_fields() {
  module_load_include('inc', 'cer', 'cer.cer');
  return _cer_collect_fields_of_type('commerce_product_reference', 'CerCommerceProductReferenceField');
}

/**
 * Implements hook_cer_preset_create().
 */
function cer_commerce_cer_preset_create(CerPreset $preset) {
  // Prevents a race condition when deleting Commerce products which are referred to
  // by Product Reference fields managed by CER.
  if ($preset->right->end() instanceof CerCommerceProductReferenceField) {
    $preset->bidirectional = FALSE;
    drupal_set_message(t('In order to prevent a race condition, bidirectionality has been disabled for this preset.'), 'warning');
  }
}
