<?php

/**
 * @file
 * Contains the CerFieldHierarchy object.
 */

class CerFieldHierarchy implements Countable {

  /**
   * @var string
   *  Cache ID for this hierarchy.
   */
  protected $cacheID;
  
  /**
   * @var array
   *  The flattened hierarchy data.
   */
  protected $hierarchy = array();

  const ROOT = 'root';

  /**
   * Constructor.
   */
  public function __construct($cacheID) {
    $this->cacheID = $cacheID;
  }

  /**
   * Returns a hierarchical_select form field array for this hierarchy.
   *
   * @return array
   */
  public function getHierarchicalSelect(array $config = array()) {
    $defaults = array(
      'params' => self::getParameters(),
      // The rest of this mirrors Hierarchical Select's defaults. However,
      // if I don't explicity specify them here, Hierarchical Select will
      // generate a lot of annoying 'undefined index' notices.
      'save_lineage' => FALSE,
      'enforce_deepest' => FALSE,
      'entity_count' => FALSE,
      'require_entity' => FALSE,
      'resizable' => TRUE,
      'level_labels' => array(
        'status' => FALSE,
      ),
      'dropbox' => array(
        'status' => FALSE,
      ),
      'editability' => array(
        'status' => FALSE,
      ),
      'render_flat_select' => FALSE,
    );
    $config = $config ? array_merge_recursive($defaults, $config) : $defaults;

    $this->cache();
    $config['module'] = 'cer';
    $config['params']['id'] = $this->cacheID;

    return array(
      '#type' => 'hierarchical_select',
      '#config' => $config,
    );
  }

  /**
   * Writes this hierarchy into the cache.
   */
  public function cache() {
    cache_set($this->cacheID, $this, 'cache', CACHE_TEMPORARY);
  }

  /**
   * Deletes this hierarchy from the cache.
   */
  public function clean() {
    cache_clear_all($this->cacheID, 'cache');
  }

  public function add($item_key, $label = NULL, $parent = CerFieldHierarchy::ROOT) {
    if (!array_key_exists($item_key, $this->hierarchy)) {
      $this->hierarchy[$item_key]['label'] = isset($label) ? $label : $item_key;
    }

    if (!isset($this->hierarchy[$parent]['children'])) {
      $this->hierarchy[$parent]['children'] = array();
    }

    if (!in_array($item_key, $this->hierarchy[$parent]['children'])) {
      $this->hierarchy[$parent]['children'][] = $item_key;
    }
  }

  public function addField(CerField $field) {
    $bundle_key = "{$field->entityType}:{$field->bundle}";

    if ($field->entityInfo('entity keys', 'bundle')) {
      $this->add($field->entityType, $field->entityInfo('label'));
      $this->add($bundle_key, $field->entityInfo('bundles', $field->bundle, 'label'), $field->entityType);
    }
    else {
      $this->add($bundle_key, $field->entityInfo('label'));
    }

    $field_key = "{$bundle_key}:{$field->name}";
    $this->add($field_key, $field->label, $bundle_key);
  }

  public function addChain(CerFieldChain $chain) {
    $parents = array();

    foreach ($chain as $field) {
      if ($field->plugin['require parent']) {
        $parent_key = implode('::', $parents);
        $field_key = "{$parent_key}::{$field}";

        $this->add($field_key, $field->label, $parent_key);
      }
      else {
        $this->addField($field);
      }

      $parents[] = $field->__toString();
    }
  }

  public function validItem($key) {
    return array_key_exists($key, $this->hierarchy);
  }

  public function getLabel($key) {
    return $this->hierarchy[$key]['label'];
  }
  
  public function getLineage($item_key, array $lineage = NULL) {
    if (empty($lineage)) {
      $lineage = array($item_key);
    }

    foreach ($this->hierarchy as $key => $item) {
      if ($key == self::ROOT) {
        continue;
      }

      if (isset($item['children']) && in_array($item_key, $item['children'])) {
        array_unshift($lineage, $key);
        $this->getLineage($key, $lineage);
      }
    }

    return $lineage;
  }

  public static function getParameters() {
    return array('id');
  }

  /**
   * Returns a cached FieldHierarchy object, or a new instance if there is no cached
   * instance with the given cache ID.
   */
  public static function get($cacheID) {
    if ($c = cache_get($cacheID)) {
      return $c->data;
    }
    else {
      $class = __CLASS__;
      return new $class($cacheID);
    }
  }

  public function getOptions($item) {
    $options = array();

    if (isset($this->hierarchy[$item]['children'])) {
      foreach ($this->hierarchy[$item]['children'] as $child) {
        if (isset($this->hierarchy[$child]['label'])) {
          $options[$child] = $this->hierarchy[$child]['label'];
        }
      }
    }

    return $options;
  }

  /**
   * @implements Countable::count().
   */  
  public function count() {
    return sizeof($this->hierarchy);
  }

}
