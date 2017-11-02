<?php

/**
 * @file
 *  Contains the CerFieldChain object.
 */

/**
 * @class
 *  Encapsulates a doubly-linked list of CerField objects.
 */
class CerFieldChain implements SeekableIterator {

  /**
   * @var array
   *  Every CerField object in the chain.
   */
  protected $chain = array();
  
  /**
   * @var integer
   */
  protected $index = 0;

  /**
   * Magic post-unserialization callback. Provides every field in the chain
   * with a reference to its parent (if any) and child (if any).
   */
  public function __wakeup() {
    foreach ($this->chain as $field) {
      if (isset($parent)) {
        $field->parent($parent)->child($field);
      }
      $parent = $field;
    }
  }

  /**
   * Generates an identifier for the chain.
   *
   * @return string
   */
  public function __toString() {
    $key = array();

    foreach ($this->chain as $field) {
      $key[] = $field->__toString();
    }

    return implode('::', $key);
  }

  /**
   * Convenience method. Returns a handler for this chain in the context of
   * the given entity.
   *
   * @return CerFieldChainHandler
   */
  public function getHandler(EntityDrupalWrapper $entity) {
    return new CerFieldChainHandler($this, $entity);
  }

  /**
   * Generates a human-readable label for the chain.
   *
   * @return string
   */
  public function getLabel() {
    $label = array();

    foreach ($this->chain as $field) {
      $label[] = $field->getLabel();
    }

    return implode(' &raquo; ', $label);
  }

  /**
   * Gets a regular expression to match field chain identifiers that this chain can
   * reference, e.g. /^node:(page|article):/
   */
  public function regex() {
    $end = $this->end();
    return '/^' . $end->getTargetType() . ':(' . implode('|', $end->getTargetBundles()) . '):/';
  }

  public function addField(CerField $field, array &$completed = NULL) {
    array_unshift($this->chain, $field);
    $this->__wakeup();

    if (isset($completed)) {
      $parents = array_map('CerField::getPlugin', $field->plugin['parents']);

      if ($parents) {
        foreach ($parents as $parent) {
          $branch = clone $this;
          $branch->addField($parent, $completed);
        }
      }
      else {
        $completed[] = $this;
      }
    }
  }
  
  public function end() {
    return end($this->chain);
  }

  /**
   * Implements SeekableIterator::seek().
   */
  public function seek($position) {
    if ($position >= 0 && $position < sizeof($this->chain)) {
      $this->index = $position;
    }
    else {
      throw new OutOfBoundsException(t('Cannot seek to invalid position %position.', array('%position' => $position)));
    }
  }

  /**
   * Implements Iterator::current().
   */
  public function current() {
    return $this->chain[$this->index];
  }

  /**
   * Implements Iterator::key().
   */
  public function key() {
    return $this->current()->__toString();
  }
  
  /**
   * Implements Iterator::next().
   */
  public function next() {
    $this->index++;
  }
  
  /**
   * Implements Iterator::rewind().
   */
  public function rewind() {
    $this->index = 0;
  }

  /**
   * Implements Iterator::valid().
   */
  public function valid() {
    return ($this->index < sizeof($this->chain));
  }

  /**
   * Returns an array of every possible field chain for every field defined in
   * hook_cer_fields().
   *
   * @return array
   */
  public static function collectAll() {
    $chains = array();

    foreach (array_keys(CerField::getPluginInfo()) as $identifier) {
      $chains = array_merge($chains, self::collect($identifier));
    }

    return $chains;
  }

  /**
   * Returns an array of every possible field chain for a single field, identified
   * by its key in hook_cer_fields().
   *
   * @return array
   */
  public static function collect($identifier) {
    $chains = array();

    $chain = new CerFieldChain();
    $chain->addField(CerField::getPlugin($identifier), $chains);

    return $chains;
  }

  /**
   * Constructs and returns a CerFieldChain object from an encoded string
   * of field plugin identifiers glued together with ::.
   *
   * @return CerFieldChain
   */
  public static function unpack($identifier) {
    $chain = new CerFieldChain();

    foreach (array_reverse(explode('::', $identifier)) as $field) {
      $chain->addField(CerField::getPlugin($field));
    }
    
    return $chain;
  }

} 
 