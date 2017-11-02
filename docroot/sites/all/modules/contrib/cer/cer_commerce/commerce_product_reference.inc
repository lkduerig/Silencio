<?php

/**
 * @file
 * Contains a CER plugin for Commerce's Product Reference fields.
 */
 
class CerCommerceProductReferenceField extends CerField {

  public function getTargetType() {
    return 'commerce_product';
  }

  public function getTargetBundles() {
    $instance = field_info_instance($this->entityType, $this->name, $this->bundle);
    $types = $instance['settings']['referenceable_types'];
    return ($types ? $types : parent::getTargetBundles());
  }

}
 