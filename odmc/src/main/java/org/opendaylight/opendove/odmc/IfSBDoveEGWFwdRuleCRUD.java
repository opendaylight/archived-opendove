/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfSBDoveEGWFwdRuleCRUD {
    public boolean egwFwdRuleExists(String ruleUUID);

    public OpenDoveEGWFwdRule getEgwFwdRule(String ruleUUID);

    public void addEgwFwdRule(String ruleUUID, OpenDoveEGWFwdRule rule);

    public List<OpenDoveEGWFwdRule> getEgwFwdRules();

    public void removeEgwFwdRule(String ruleUUID);
}
