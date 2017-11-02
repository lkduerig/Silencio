<?php

/**
 * @file
 * Contains the CerPreset object.
 */

/**
 * @class
 *  Encapsulates a single CER preset, which consists of two CerFieldChain objects.
 */
class CerPreset extends StdClass {

  /**
   * This is always computed by __get(). But it needs to be here so that
   * property_exists($preset, 'id') will return TRUE, which is how
   * drupal_write_record() finds record properties.
   */
  protected $id;

  /**
   * @var integer
   * Set by the CTools Export API.
   */
  protected $export_type;

  /**
   * @var boolean
   * Set by hook_cer_default_presets().
   */
  protected $disabled = FALSE;

  /**
   * @var CerFieldChain
   */
  protected $a;

  /**
   * @var CerFieldChain
   */
  protected $b;

  /**
   * @var boolean
   */
  protected $bidirectional = TRUE;

  /**
   * @var integer
   */
  protected $weight = 0;
  
  /**
   * @var boolean
   * This flag is only applicable to presets that live in the database. (@see $this->enabled())
   */
  protected $enabled = TRUE;

  public function __get($property) {
    switch ($property) {
      case 'id':
        return MD5("{$this->left}*{$this->right}");

      case 'left':
        return $this->a;

      case 'right':
        return $this->b;

      // This constant is used by ctools_export_set_object_status().
      case 'table':
        return 'cer';

      default:
        return $this->{$property};
    }
  }

  public function __set($property, $value) {
    switch ($property) {
      case 'a':
      case 'left':
        $this->setChain($this->a, $value);
        break;

      case 'b':
      case 'right':
        $this->setChain($this->b, $value);
        break;

      case 'bidirectional':
      case 'disabled':
      case 'enabled':
        $this->{$property} = (boolean) $value;
        break;

      case 'export_type':
      case 'weight':
        $this->{$property} = (int) $value;
        break;

      default:
        break;
    }
  }

  /**
   * CTools uses both property_exists() and isset() when exporting presets,
   * so this normalizes them.
   *
   * @return boolean
   */
  public function __isset($property) {
    return property_exists($this, $property);
  }

  /**
   * Setter for $this->a and $this->b.
   *
   * @param $property
   *  A reference to the field to set.
   *
   * @param $chain
   *  Either a CerFieldChain object, or a field chain encoded as a string,
   *  like in {cer}.a and {cer}.b.
   */
  protected function setChain(&$property, $chain) {
    if ($chain instanceof CerFieldChain) {
      $property = $chain;
    }
    elseif (is_string($chain) && $chain) {
      $property = CerFieldChain::unpack($chain);
    }
  }

  /**
   * Checks if this preset is enabled.
   *
   * @return boolean
   */
  public function enabled() {
    ctools_include('export');

    // If this preset lives in the database, {cer}.enabled is its canonical status flag.
    // Otherwise, we check the default_cer variable, which CTools maintains for us,
    // and if it doesn't mention this preset, return the inverse of $this->disabled,
    // which is exported in hook_cer_default_presets().
    if ($this->export_type & EXPORT_IN_DATABASE) {
      return $this->enabled;
    }
    else {
      $id = $this->__get('id');
      $status = variable_get('default_cer', array());

      return (isset($status[$id]) ? $status[$id] : !$this->disabled);
    }
  }

  /**
   * Returns the inverse of this preset (i.e., a new preset with the left and
   * right field chains flipped). The inverted preset isn't saved to the DB or
   * anything, it exists only in memory.
   *
   * @return CerPreset
   */
  public function invert() {
    ctools_include('export');

    $inverse = ctools_export_crud_new('cer', FALSE);

    $inverse->left = $this->right;
    $inverse->right = $this->left;
    $inverse->bidirectional = $this->bidirectional;
    
    return $inverse;
  }

}
