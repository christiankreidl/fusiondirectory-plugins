<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2012-2023 FusionDirectory

  Author : Thibault Paul M Dockx

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

class tasksNotificationsConf extends simplePlugin
{
  static function plInfo (): array
  {
    return [
      'plShortName'    => _('Notifications'),
      'plDescription'  => _('Tasks Notifications Configuration'),
      'plObjectType'   => ['configuration'],
      'plObjectClass'  => ['fdTasksNotificationsConf'],
      'plProvidedAcls' => parent::generatePlProvidedAcls(static::getAttributesInfo())
    ];
  }

  // The main function : information about attributes
  static function getAttributesInfo (): array
  {
    return [
      'aidaAccountInformation' => [
        'name'  => _('Notifications Tasks Configuration'),
        'attrs' => [
          new SetAttribute(
            new StringAttribute(
              _('Attributes name'), _('Attributes name in ACL format'),
              'fdTasksNotificationsConfAttributes', FALSE, ''
            ),
          ),
        ]
      ]
    ];
  }

  function __construct ($dn = NULL, $object = NULL, $parent = NULL, $mainTab = FALSE)
  {
    global $config;
    parent::__construct($dn, $object, $parent, $mainTab);
  }

  protected function shouldSave (): bool
  {
    return parent::shouldSave();
  }

  /*
   * Save method, by interface rule, must remain public
   */
  public function save (): array
  {
    $errors = parent::save();
    return $errors;
  }
}
