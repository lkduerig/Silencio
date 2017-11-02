<?php

/**
 * Page callback listing all CER presets.
 */
function cer_preset_list(array $form, array &$form_state) {
  // The table element type is provided by the table_element module, which
  // is a backport from Drupal 8.
  $form['cer'] = array(
    '#type' => 'table',
    '#header' => array(
      t('Left Field'),
      t('Right Field'),
      t('Bidirectional'),
      t('Weight'),
      t('Operations'),
    ),
    '#tabledrag' => array(
      array('order', 'sibling', 'weight'),
    ),
    '#empty' => t('No presets have been created yet.'),
  );

  ctools_include('export');
  foreach (ctools_export_crud_load_all('cer') as $id => $preset) {
    $form['cer'][$id] = array(
      'left' => array(
        '#markup' => sprintf('<strong>%s</strong>: %s', $preset->left->end()->fieldTypeLabel, $preset->left->getLabel()),
      ),
      'right' => array(
        '#markup' => sprintf('<strong>%s</strong>: %s', $preset->right->end()->fieldTypeLabel, $preset->right->getLabel()),
      ),
      'bidirectional' => array(
        '#type' => 'checkbox',
        '#default_value' => $preset->bidirectional,
      ),
      'weight' => array(
        '#type' => 'weight',
        '#default_value' => $preset->weight,
        '#attributes' => array(
          'class' => array('weight'),
        ),
      ),
      'operations' => array(
        '#theme' => 'links__ctools_dropbutton',
        '#links' => array(
          'toggle' => array(
            'title' => ($preset->enabled() ? t('disable') : t('enable')),
            'href' => "admin/config/content/cer/presets/toggle/{$id}",
          ),
          'revert_delete' => array(
            'title' => ($preset->export_type & EXPORT_IN_CODE ? t('revert') : t('delete')),
            'href' => "admin/config/content/cer/presets/revert-delete/{$id}",
          ),
          'export' => array(
            'title' => t('export'),
            'href' => "admin/config/content/cer/presets/export/{$id}",
          ),
        ),
      ),
      '#attributes' => array(
        'class' => array('draggable'),
      ),
      '#preset' => $preset,
    );

    // Only one-directional presets can be flipped in the UI.
    if (! $preset->bidirectional) {
      $form['cer'][$id]['operations']['#links']['invert'] = array(
        'title' => t('invert'),
        'href' => "admin/config/content/cer/presets/invert/{$id}",
      );
    }

    $form['actions'] = array(
      'submit' => array(
        '#type' => 'submit',
        '#value' => t('Save Changes'),
      ),
      '#type' => 'actions',
    );
  }

  return $form;
}

/**
 * Submit handler. Writes any preset whose weight has been changed into the database,
 * possibly overriding it.
 */
function cer_preset_list_submit(array $form, array &$form_state) {
  foreach ($form_state['values']['cer'] as $id => $values) {
    $write = FALSE;

    $preset = $form['cer'][$id]['#preset'];
    if ($values['weight'] != $preset->weight) {
      $preset->weight = $values['weight'];
      $write = TRUE;
    }
    if ($values['bidirectional'] != $preset->bidirectional) {
      $preset->bidirectional = $values['bidirectional'];
      $write = TRUE;
    }
    
    if ($write) {
      ctools_export_crud_save('cer', $preset);
    }
  }

  drupal_set_message(t('Saved changes.'));
}

/**
 * Page callback to create a new preset.
 */
function cer_add_preset(array $form, array &$form_state) {
  drupal_set_title(t('Add a CER preset'));

  // This form depends on Hierarchical Select, so if it's not installed, stop here
  // with an error.
  if (! module_exists('hierarchical_select')) {
    drupal_set_message(t('You must install the <a href="http://www.drupal.org/project/hierarchical_select">Hierarchical Select</a> module in order to create CER presets.'), 'error');
    return $form;
  }

  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
    '#submit' => array(
      '_cer_add_preset_cancel',
    ),
    '#weight' => 5,
  );

  // Either create a new hierarchy to choose the left field, or load it from
  // the cache if the form is being rebuilt.
  $left = CerFieldHierarchy::get('cer_left');

  // If the hierarchy is empty, add every available field chain to it.
  if (sizeof($left) == 0) {
    foreach (CerFieldChain::collectAll() as $chain) {
      $left->addChain($chain);
    }
  }

  // If the hierarchy now has data, generate a hierarchical_select field and add
  // it to the form. Otherwise, fail with an error message.
  if (sizeof($left) > 0) {
    $form['left'] = $left->getHierarchicalSelect(array('save_lineage' => TRUE));
    $form['left']['#required'] = TRUE;
    $form['left']['#title'] = t('Choose a field...');
  }
  else {
    drupal_set_message(t('There are no reference fields available.'), 'warning');
    return $form;
  }

  // One the user has chosen a left field and clicked Continue, we can find the
  // possible right fields. (@see _cer_add_preset_continue())
  if (isset($form_state['storage']['cer']['left'])) {
    // Fetch the left field chain from storage, and get its full identifier
    // and a regex we can use to match it against right fields.
    $left = $form_state['storage']['cer']['left'];
    $left_identifier = $left->__toString();
    $left_re = $left->regex();

    // Create, or get from the cache, a hierarchy for the right field chain.
    $right = CerFieldHierarchy::get('cer_right');

    // Loop through every possible field chain and test it against the left
    // chain's regex. If it matches, we can add the chain to the right hierarchy.
    foreach (CerFieldChain::collectAll() as $chain) {
      if (preg_match($left_re, $chain->__toString()) && preg_match($chain->regex(), $left_identifier)) {
        $right->addChain($chain);
      }
    }

    // If there's data in the right hierarchy, we have corresponding fields to choose from,
    // so display the other hierarchical_select field and advanced options. Otherwise,
    // display an error and start over.
    if (sizeof($right) > 0) {
      $form['left'] = array(
        '#type' => 'markup',
        '#markup' => sprintf('%s', $left->getLabel()),
      );

      $form['right'] = $right->getHierarchicalSelect(array('save_lineage' => TRUE));
      $form['right']['#title'] = t('Choose a corresponding field...');

      $form['advanced'] = array(
        '#type' => 'fieldset',
        '#title' => t('Advanced'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      );
      $form['advanced']['enabled'] = array(
        '#type' => 'checkbox',
        '#title' => t('Enabled'),
        '#default_value' => TRUE,
      );
      $form['advanced']['bidirectional'] = array(
        '#type' => 'checkbox',
        '#title' => t('Bidirectional'),
        '#default_value' => TRUE,
        '#description' => t('If checked, a change to either field will trigger a reaction on the other field. Otherwise, only a change to the first field will trigger a reaction on the other field.'),
      );

      $form['actions']['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Save Preset'),
      );
    }
    else {
      drupal_set_message(t('There are no fields which can correspond with !left.', array('!left' => $left->getLabel())), 'warning');
    }
  }
  else {
    $form['actions']['continue'] = array(
      '#type' => 'submit',
      '#value' => t('Continue'),
      '#submit' => array(
        '_cer_add_preset_continue',
      ),
    );
  }

  return $form;
}

/**
 * Submit handler. Stores the selected left field in $form_state['storage'] and rebuilds
 * the form so the user can choose the right field.
 */
function _cer_add_preset_continue(array $form, array &$form_state) {
  $form_state['storage']['cer']['left'] = CerFieldChain::unpack(end($form_state['values']['left']));  
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit handler. Deletes the left and right field hierarchies from the cache and
 * redirects to the preset list.
 */
function _cer_add_preset_cancel(array $form, array &$form_state) {
  CerFieldHierarchy::get('cer_left')->clean();
  CerFieldHierarchy::get('cer_right')->clean();

  $form_state['redirect'] = 'admin/config/content/cer';
}

/**
 * Submit handler. Saves a preset to the database.
 */
function cer_add_preset_submit(array $form, array &$form_state) {
  ctools_include('export');

  $preset = ctools_export_crud_new('cer', FALSE);

  // The left chain was stored in $form_state['storage'], fully unpacked.
  // The user will have just selected the right chain, though, so we'll need
  // to unpack it now, which might throw an exception if there's an invalid
  // plugin identifier in the chain.
  $preset->left = $form_state['storage']['cer']['left'];
  $preset->right = CerFieldChain::unpack(end($form_state['values']['right']));

  $preset->bidirectional = $form_state['values']['bidirectional'];
  $preset->enabled = TRUE;

  // Allow modules to react and/or modify the preset before we save it.
  module_invoke_all('cer_preset_create', $preset);

  if (ctools_export_crud_save('cer', $preset)) {
    drupal_set_message(t('The preset was saved successfully.'));
  }
  else {
    drupal_set_message(t('An error occurred while trying to save the preset.'), 'error');
  }

  // Clean up and redirect
  _cer_add_preset_cancel($form, $form_state);
}

/**
 * Page callback to toggle a preset from the main administration page.
 */
function cer_preset_toggle(CerPreset $preset) {
  ctools_include('export');
  // This will invoke hook_cer_preset_toggle(). @see cer_preset_set_status().
  ctools_export_crud_set_status('cer', $preset, !$preset->enabled());

  drupal_goto('admin/config/content/cer');
}

/**
 * Page callback to invert a preset from the main administration page.
 */
function cer_preset_invert(CerPreset $original) {
  if (! $original->bidirectional) {
    ctools_include('export');

    $inverse = $original->invert();
    // Don't invoke hook_cer_preset_create() here, because we don't want some
    // clever-dick to re-invert the preset before it gets saved.
    ctools_export_crud_save('cer', $inverse);

    // There's no point in having two presets that are inverses of each other,
    // so delete and/or disable the original.
    if ($original->export_type & EXPORT_IN_DATABASE) {
      ctools_export_crud_delete('cer', $original);

      // If the original was a database-only preset, invoke hook_cer_preset_delete().
      if (! ($original->export_type & EXPORT_IN_CODE)) {
        module_invoke_all('cer_preset_delete', $original);
      }
    }
    if ($original->export_type & EXPORT_IN_CODE) {
      // This will invoke hook_cer_preset_toggle().
      ctools_export_crud_set_status('cer', $original, FALSE);
    }
  }
  else {
    drupal_set_message(t('You cannot invert a bidirectional preset.'), 'error');
  }

  drupal_goto('admin/config/content/cer');
}

/**
 * Form builder. Provides a text area for exported preset code.
 */
function cer_import_preset(array $form, array &$form_state) {
  $form['code'] = array(
    '#type' => 'textarea',
    '#required' => TRUE,
    '#rows' => 10,
  );

  $form['actions'] = array(
    'import' => array(
      '#type' => 'submit',
      '#value' => t('Import'),
    ),
    '#type' => 'actions',
  );

  $form['#redirect'] = 'admin/config/content/cer';

  return $form;
}

/**
 * Submit handler. Imports a preset from code.
 */
function cer_import_preset_submit(array $form, array &$form_state) {
  ctools_include('export');
  eval($form_state['values']['code']);

  if (isset($preset) && $preset instanceof CerPreset) {
    if (ctools_export_crud_save('cer', $preset)) {
      drupal_set_message(t('The preset was imported successfully.'));
    }
    else {
      drupal_set_message(t('An error occurred while trying to save the preset.'), 'error');
    }
  }
  else {
    drupal_set_message(t('The exported preset is not valid.'), 'error');
  }
}

/**
 * Form builder. Exports a preset in a textarea.
 */
function cer_export_preset(array $form, array &$form_state, CerPreset $preset) {
  ctools_include('export');
  
  $form['export'] = array(
    '#type' => 'textarea',
    '#rows' => 10,
    '#default_value' => ctools_export_object('cer', $preset),
  );

  return $form;
}

/**
 * Confirmation form to delete a preset.
 */
function cer_revert_delete_preset(array $form, array &$form_state, CerPreset $preset) {
  ctools_include('export');
  
  $variables = array();
  
  if ($preset->export_type & EXPORT_IN_CODE) {
    $variables['!verb'] = t('revert');
    $info = t('Any customizations will be lost.');
  }
  else {
    $variables = array(
      '!left' =>
        $preset->left->getLabel(),
      '!right' =>
        $preset->right->getLabel(),
      '!verb' =>
        t('delete'),
    );
    $info = t('<i>!left</i> will no longer correspond with <i>!right</i>.', $variables);
  }

  return confirm_form($form, t('Are you sure you want to !verb this preset?', $variables), 'admin/config/content/cer', $info, ucFirst($variables['!verb']));
}

function cer_revert_delete_preset_submit(array $form, array &$form_state) {
  $preset = $form_state['build_info']['args'][0];

  // The form builder includes export.inc, so this function should exist.
  ctools_export_crud_delete('cer', $preset);

  // Only invoke hook_cer_preset_delete() if this preset isn't defined in code.
  if (! ($preset->export_type & EXPORT_IN_CODE)) {
    module_invoke_all('cer_preset_delete', $preset);
  }

  $variables = array(
    '!verb' =>
      ($preset->export_type & EXPORT_IN_CODE ? t('reverted') : t('deleted')),
  );
  drupal_set_message(t('The preset was !verb.', $variables));

  $form_state['redirect'] = 'admin/config/content/cer';
}

/**
 * Allows batch updating of existing entities.
 */
function cer_bulk_update_form(array $form, array &$form_state) {
  $form['type'] = array(
    '#type' => 'select',
    '#title' => t('Entity type'),
    '#required' => TRUE,
    '#options' => array(),
    '#description' => t('Select the entity type that you want to update.'),
  );
  foreach (entity_get_info() as $type => $class) {
    $form['type']['#options'][$type] = $class['label'];
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

/**
 * The update form. Allows bulk updating of current entities.
 */
function cer_bulk_update_form_submit($form, &$form_state) {
  $batch = array(
    'finished' => 'cer_batch_update_existing_finished',
    'title' => t('Processing'),
    'init_message' => t('Preparing to update corresponding entity references for existing entities...'),
    'progress_message' => t('Processing entities...'),
    'error_message' => t('Corresponding entity references - existing entity update has encountered an error.'),
    'operations' => array(),
  );

  $entity_type = $form_state['values']['type'];
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', $entity_type);
  if ($entity_type == 'user') {
    $query->entityCondition('entity_id', 0, '>');
  }
  $result = $query->execute();

  if (isset($result[$entity_type])) {
    foreach (array_keys($result[$entity_type]) as $entity_id) {
      $batch['operations'][] = array('cer_processing_entity', array('update', $entity_id, $entity_type));
    }
  }

  batch_set($batch);
}
