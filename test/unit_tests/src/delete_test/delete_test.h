/*******************************************************************************
 *
 * Copyright 2017-2018 Tobias Neumann	<t.neumann@fh-aachen.de>
 *
******************************************************************************/

/*  This file is part of mapit.
 *
 *  Mapit is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mapit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with mapit.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __DELETE_TEST_H
#define __DELETE_TEST_H

#include <QTest>
#include "../repositorycommon.h"

class DeleteTest : public RepositoryCommon
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void initTestCase();
    void cleanupTestCase();

    void test_delete_entity_data();
    void test_delete_entity();
    void test_delete_tree_data();
    void test_delete_tree();
    void test_delete_sub_entity_data();
    void test_delete_sub_entity();
    void test_delete_sub_tree_data();
    void test_delete_sub_tree();

    void test_delete_entities_and_trees_mixed_data();
    void test_delete_entities_and_trees_mixed();
private:
    void add_bunny(std::shared_ptr<mapit::Workspace> workspace, std::string path);
};

#endif
