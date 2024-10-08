<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)

  Copyright (C) 2019-2020 FusionDirectory

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

class archiveManagement extends management
{
  protected $skipCpHandler      = TRUE;
  public static $skipSnapshots  = TRUE;

  /* Default columns */
  public static $columns = [
    ['ObjectTypeColumn',          []],
    ['LdapGeneralizedTimeColumn', ['attributes' => 'fdArchivedDateTime',   'label' => 'Time']],
    ['ObjectTypeColumn',          ['attributes' => 'fdArchivedObjectType', 'label' => 'Type']],
    ['LinkColumn',                ['attributes' => 'fdArchivedOriginDn',   'label' => 'Origin']],
    ['ActionsColumn',             ['label' => 'Actions']],
  ];

  public static function plInfo (): array
  {
    return [
      'plShortName'   => _('Archived objects'),
      'plTitle'       => _('Archived objects'),
      'plDescription' => _('Browse archived objects'),
      'plIcon'        => 'geticon.php?context=mimetypes&icon=application-x-archive&size=48',
      'plSection' => ['reporting' => ['name' => _('Reporting'), 'priority' => 5]],
      'plPriority'    => 4,
      'plManages'     => ['archivedObject'],
    ];
  }

  function __construct ()
  {
    parent::__construct();

    $types = [];
    foreach (archivedObject::listArchiveTypes() as $type) {
      $infos = objects::infos($type);
      $types[$type] = [
        'name'    => $infos['name'],
        'icon'    => $infos['icon'],
        'filter'  => '(fdArchivedObjectType='.$type.')',
      ];
    }
    $this->filter->addElement(new CheckBoxesFilterElement($this->filter, _('Archived types'), $types, '|'));

    $this->filter->addElement(new DateFilterElement($this->filter, 'fdArchivedDateTime', _('Date')));
  }

  protected function setUpListing ()
  {
    parent::setUpListing();
    /* Newer archives first */
    $this->listing->setSortColumn(1, TRUE);
  }

  protected function configureActions ()
  {
    parent::configureActions();

    /* Remove create */
    $removeActions = ['new'];
    foreach ($removeActions as $action) {
      foreach ($this->actions[$action]->listActions() as $actionName) {
        unset($this->actionHandlers[$actionName]);
      }
      unset($this->actions[$action]);
    }
    unset($this->actions['removeConfirmed']);

    $this->actions['edit']->setInMenu(FALSE);
  }
}
